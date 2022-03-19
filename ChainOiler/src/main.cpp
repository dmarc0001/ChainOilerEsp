#include "main.hpp"
#include "MainWork.hpp"

/**
 * @brief Der Programmstart
 *
 */
void app_main()
{
  TaskHandle_t workerHandle;
  // TaskHandle_t mainThreadHandle = xTaskGetIdleTaskHandle();
  std::string threadName = std::string("mainThjread");

  ChOiler::MainWorker::init();
  auto ret = xTaskCreate(ChOiler::MainWorker::run, threadName.c_str(), Prefs::THREAD_STACK_SIZE, nullptr, tskIDLE_PRIORITY, &workerHandle);
  if (pdPASS == ret)
  {
    ESP_LOGI("main", "worker task started...");
  }
  configASSERT(workerHandle);
  // ChOiler::MainWorker::run(workerHandle);

  while (true)
  {
    vTaskDelay(pdMS_TO_TICKS(1500));
    rtc_wdt_feed();
    // esp_task_wdt_reset();
  }
  // Use the handle to delete the task.
  if (workerHandle != nullptr)
  {
    vTaskDelete(workerHandle);
  }
}
