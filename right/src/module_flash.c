#include "module_flash.h"

module_flash_state_t ModuleFlashState = ModuleFlashState_Idle;
bool ModuleFlashBusy = false;
uint8_t ModuleFlashErrorCode = 0;
