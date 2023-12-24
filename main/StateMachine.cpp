#include "state.h"
#include "config.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/ringbuf.h>
#include "shared.h"
#include <esp_event_base.h>
#include <esp_event.h>

ESP_EVENT_DEFINE_BASE(STATE_MACHINE_EVENT);

StateMachine::StateMachine(std::shared_ptr<Stepper> aStepper, std::shared_ptr<esp_event_loop_handle_t> aUIEventLoop) : currentState(State::Stopped), currentSpeedState(SpeedState::Normal) {
	myUiEventLoop = aUIEventLoop;
    myStepper = aStepper;
	myRef = this;

	esp_event_loop_args_t loopArgs = {
		.queue_size = 16,
		.task_name = "StateMachineEventLoop", // task will be created
		.task_priority = uxTaskPriorityGet(NULL),
		.task_stack_size = 3072*2,
		.task_core_id = tskNO_AFFINITY};

	myEventLoop = std::make_shared<esp_event_loop_handle_t>();
	auto loop = myEventLoop.get();
	ESP_ERROR_CHECK(esp_event_loop_create(&loopArgs, loop));
	//myEventLoop = xRingbufferCreate(sizeof(Event)*1024, RINGBUF_TYPE_NOSPLIT);
	ASSERT_MSG(myEventLoop, "StateMachine", "Failed to create event loop");

	//myUpdateSpeedEventLoop = xRingbufferCreate(sizeof(UpdateSpeedEventData)*1024*3, RINGBUF_TYPE_NOSPLIT);;
	//ASSERT_MSG(myUpdateSpeedEventLoop, "StateMachine", "Failed to create update speed ringbuf");
	
    ESP_LOGI("state.cpp", "State Machine init complete");
}

void StateMachine::Start()
{
//	esp_event_handler_register()
	auto loop = myEventLoop.get();
	ESP_ERROR_CHECK(esp_event_handler_instance_register_with(*loop, STATE_MACHINE_EVENT, ESP_EVENT_ANY_ID, ProcessEventLoopIteration, myRef, nullptr));
//esp_event_handler_instance_t
	xTaskCreate(StateMachine::EventLoopRunnerTask, "StateMachineEventLoop", 3072*5, this, uxTaskPriorityGet(NULL) + 1, &myEventLoopTaskHandle);

	//	xTaskCreate(&StateMachine::ProcessEventQueueTask, "ProcessEventQueueTask", 2048 * 24, this, 5, NULL);
	//	xTaskCreate(&StateMachine::ProcessUpdateSpeedQueueTask, "ProcessUpdateSpeedQueueTask", 1024 * 24, this, 5, NULL);
}

void StateMachine::EventLoopRunnerTask(void *stateMachine)
{
	StateMachine *sm = static_cast<StateMachine *>(stateMachine);
	while (1)
	{
		esp_event_loop_run(*sm->myEventLoop, 100);
		vTaskDelay(10);
	}
}

void StateMachine::ProcessEventLoopIteration(void *stateMachine, esp_event_base_t base, int32_t id, void *payload) {
	//ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	StateMachine *sm = static_cast<StateMachine *>(stateMachine);
	ASSERT_MSG(sm, "ProccessEventQueueTask", "StateMachine was null on start of task");
	
	Event event = static_cast<Event>(id);
	
	EventData *eventData = static_cast<EventData *>(payload);

	sm->ProcessEvent(event, eventData);

	//delete eventData;
}

void StateMachine::MoveLeftAction() {
    if(currentState == State::MovingLeft) {
        return;
    }

    ESP_LOGI("state.cpp", "Left pressed");
    currentState = State::MovingLeft;
    myStepper->MoveLeft();
    //ESP_LOGI("state.cpp", "Done requesting stepper move left");
}

void StateMachine::MoveRightAction() {
    if(currentState == State::MovingRight) {
        return;
    }

    ESP_LOGI("state.cpp", "Right pressed");
    currentState = State::MovingRight;
    myStepper->MoveRight();
    //ESP_LOGI("state.cpp", "Done requesting stepper move right");
}

void StateMachine::RapidSpeedAction() {
    if(currentSpeedState == SpeedState::Rapid) {
        return;
    }

    ESP_LOGI("state.cpp", "Rapid pressed");
    currentSpeedState = SpeedState::Rapid;
    myStepper->SetRapidSpeed();
    //ESP_LOGI("state.cpp", "Done requesting stepper set rapid speed");
}

void StateMachine::NormalSpeedAction() {
    if(currentSpeedState == SpeedState::Normal) {
        return;
    }

    ESP_LOGI("state.cpp", "Rapid released");
    currentSpeedState = SpeedState::Normal;
    myStepper->SetNormalSpeed();
    //ESP_LOGI("state.cpp", "Done requesting stepper set normal speed");
}

void StateMachine::CheckIfStoppedTask(void* params) {
	StateMachine* sm = static_cast<StateMachine*>(params);
	ASSERT_MSG(sm, "CheckIfStoppedTask", "StateMachine was null on start of task");
	std::shared_ptr<esp_event_loop_handle_t> evht = sm->GetEventLoop();
	ASSERT_MSG(evht, "CheckIfStoppedTask", "Event ringbuf was null on start of task");
	bool isStopped = false;
	while(!isStopped) {
		vTaskDelay(pdMS_TO_TICKS(10));
		
		if(sm->myStepper->IsStopped()) {
			ESP_ERROR_CHECK(esp_event_post_to(*evht, STATE_MACHINE_EVENT, static_cast<int32_t>(Event::SetStopped), nullptr, sizeof(nullptr), pdMS_TO_TICKS(250)));
			isStopped = true;
			break;
		}
	}
	vTaskDelete(NULL);
}

void StateMachine::CreateStoppingTask() {
	xTaskCreatePinnedToCore(CheckIfStoppedTask, "processing-stopped", 24000, this, 11, nullptr, 1);
}

void StateMachine::StopLeftAction() {
    ESP_LOGI("state.cpp", "Stopping");
    currentState = State::StoppingLeft;
    myStepper->Stop();
	CreateStoppingTask();
	//ESP_LOGI("state.cpp", "Done requesting stepper stop");
}

void StateMachine::StopRightAction() {
    ESP_LOGI("state.cpp", "Stopping");
    currentState = State::StoppingRight;
    myStepper->Stop();
	CreateStoppingTask();
    //ESP_LOGI("state.cpp", "Done requesting stepper stop");
}

bool StateMachine::ProcessEvent(Event event, EventData* eventPayload) {
    switch (currentState) {
        case State::Stopped:
            //ESP_LOGI("state.cpp", "State is stopped");
            if (event == Event::LeftPressed) {
                MoveLeftAction();
				esp_event_post_to(*myUiEventLoop, UI_QUEUE_EVENT, static_cast<int32_t>(UIEvent::MoveLeft), nullptr, sizeof(nullptr), pdMS_TO_TICKS(250));
				return true;
			} else if (event == Event::RightPressed) {
                MoveRightAction();
				esp_event_post_to(*myUiEventLoop, UI_QUEUE_EVENT, static_cast<int32_t>(UIEvent::MoveRight), nullptr, sizeof(nullptr), pdMS_TO_TICKS(250));
				return true;
            }
            break;

        case State::MovingLeft:
            if (event == Event::LeftReleased) {
                StopLeftAction();
				esp_event_post_to(*myUiEventLoop, UI_QUEUE_EVENT, static_cast<int32_t>(UIEvent::Stopping), nullptr, sizeof(nullptr), pdMS_TO_TICKS(250));
				return true;
            } 
            break;

        case State::MovingRight:
            if (event == Event::RightReleased) {
				StopRightAction();
				esp_event_post_to(*myUiEventLoop, UI_QUEUE_EVENT, static_cast<int32_t>(UIEvent::Stopping), nullptr, sizeof(nullptr), pdMS_TO_TICKS(250));
				return true;
            } 
            break;

        //if we are stopping but we ask to resume in the same direction, we can move again immediately.
        //may need to force clear the queue or something.
        case State::StoppingLeft:
			if (event == Event::LeftPressed)
			{
				MoveLeftAction();
				esp_event_post_to(*myUiEventLoop, UI_QUEUE_EVENT, static_cast<int32_t>(UIEvent::MoveLeft), nullptr, sizeof(nullptr), pdMS_TO_TICKS(250));
				return true;
			}
			else if (event == Event::RightPressed)
			{
				// ignore, gotta wait till we're stopped first.
				return false;
			}
			else if (event == Event::SetStopped)
			{
				currentState = State::Stopped;
				esp_event_post_to(*myUiEventLoop, UI_QUEUE_EVENT, static_cast<int32_t>(UIEvent::Stopped), nullptr, sizeof(nullptr), pdMS_TO_TICKS(250));
				ESP_LOGI("state.cpp", "Stopped");
			} 
			break;

		case State::StoppingRight:
            if (event == Event::RightPressed) {
                MoveRightAction();
				esp_event_post_to(*myUiEventLoop, UI_QUEUE_EVENT, static_cast<int32_t>(UIEvent::MoveRight), nullptr, sizeof(nullptr), pdMS_TO_TICKS(250));
            } 
            else if (event == Event::LeftPressed) {
                //Ignore, gotta wait till we're stopped first.
				return false;
			}
			else if (event == Event::SetStopped)
			{
				currentState = State::Stopped;
				esp_event_post_to(*myUiEventLoop, UI_QUEUE_EVENT, static_cast<int32_t>(UIEvent::Stopped), nullptr, sizeof(nullptr), pdMS_TO_TICKS(250));
				ESP_LOGI("state.cpp", "Stopped");
			}
            break; 

        default:    
            break;
    }

    switch(event) {
        case Event::RapidPressed:
			RapidSpeedAction();
			esp_event_post_to(*myUiEventLoop, UI_QUEUE_EVENT, static_cast<int32_t>(UIEvent::RapidSpeed), nullptr, sizeof(nullptr), pdMS_TO_TICKS(250));
			break;
		case Event::RapidReleased:
			NormalSpeedAction();
			esp_event_post_to(*myUiEventLoop, UI_QUEUE_EVENT, static_cast<int32_t>(UIEvent::NormalSpeed), nullptr, sizeof(nullptr), pdMS_TO_TICKS(250));
			break;
		case Event::UpdateRapidSpeed: 
		{
			UpdateSpeedEventData* eventData = dynamic_cast<UpdateSpeedEventData*>(eventPayload);
			ASSERT_MSG(eventData, "StateMachine", "Failed to cast event data to UpdateSpeedEventData");

			int16_t speed = eventData->mySpeed;
			//auto rapidSpeed = eventData->myRapidSpeed;
			ESP_LOGI("StateMachine", "Updating rapid speed to %d", speed);

			myStepper->UpdateRapidSpeed(speed);

			UIEventData *uiEventData = new UIEventData(myStepper->GetTargetSpeed());
			esp_event_post_to(*myUiEventLoop, UI_QUEUE_EVENT, static_cast<int32_t>(UIEvent::SetSpeed), uiEventData, sizeof(UIEventData), pdMS_TO_TICKS(250));
			break;
		}
		case Event::UpdateNormalSpeed: 
		{
			UpdateSpeedEventData *eventData = dynamic_cast<UpdateSpeedEventData *>(eventPayload);
			ASSERT_MSG(eventData, "StateMachine", "Failed to cast event data to UpdateSpeedEventData");

			int16_t speed = eventData->mySpeed;
			// auto rapidSpeed = eventData->myRapidSpeed;
			ESP_LOGI("StateMachine", "Updating normal speed to %d", speed);

			myStepper->UpdateNormalSpeed(speed);

			UIEventData *uiEventData = new UIEventData(myStepper->GetTargetSpeed());
			esp_event_post_to(*myUiEventLoop, UI_QUEUE_EVENT, static_cast<int32_t>(UIEvent::SetSpeed), uiEventData, sizeof(UIEventData), pdMS_TO_TICKS(250));
			break;
		}
		default:
			break;
    }

	return true;
}

