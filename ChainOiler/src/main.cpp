#include "main.hpp"
#include "MainWork.hpp"

void app_main()
{
  ChOiler::MainWorker::init();
  ChOiler::MainWorker::run();
}
