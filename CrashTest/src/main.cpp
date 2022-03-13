#include "main.hpp"
#include "MainWork.hpp"

/**
 * @brief Der Programmstart
 *
 */
void app_main()
{
  static ChOiler::TaskInfos threadInfos;

  threadInfos.threadName = std::string("mainThjread");

  ChOiler::MainWorker::init();
  auto ret = xTaskCreate(ChOiler::MainWorker::run, threadInfos.threadName.c_str(), Prefs::THREAD_STACK_SIZE, &threadInfos, tskIDLE_PRIORITY, &threadInfos.workerHandle);
  if (pdPASS == ret)
  {
    ESP_LOGI("main", "worker task started...");
  }
  configASSERT(threadInfos.workerHandle);
  // ChOiler::MainWorker::run(workerHandle);

  while (true)
  {
    vTaskDelay(pdMS_TO_TICKS(1500));
  }
  // Use the handle to delete the task.
  if (threadInfos.workerHandle != nullptr)
  {
    vTaskDelete(threadInfos.workerHandle);
  }
}
