#include "main.hpp"
#include "MainWork.hpp"

/**
 * @brief Der Programmstart
 * 
 */
void app_main()
{
  ChOiler::MainWorker::init();
  ChOiler::MainWorker::run();
}
