#include "main.hpp"
#include "MainWork.hpp"

/**
 * @brief Der Programmstart
 *
 */
void app_main()
{
  //
  // Hardware und Speicher initialisieren
  //
  ChOiler::MainWorker::init();
  //
  // laufe bis in die Unendlichkeit...
  //
  ChOiler::MainWorker::run();
}
