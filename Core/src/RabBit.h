#pragma once

/*
* Welcome to the:
*  _____            _       ____    _   _       ______                   _
* |  __ \          | |     |  _ \  (_) | |     |  ____|                 (_)
* | |__) |   __ _  | |__   | |_) |  _  | |_    | |__     _ __     __ _   _   _ __     ___
* |  _  /   / _` | | '_ \  |  _ <  | | | __|   |  __|   | '_ \   / _` | | | | '_ \   / _ \
* | | \ \  | (_| | | |_) | | |_) | | | | |_    | |____  | | | | | (_| | | | | | | | |  __/
* |_|  \_\  \__,_| |_.__/  |____/  |_|  \__|   |______| |_| |_|  \__, | |_| |_| |_|  \___|
*                                                                 __/ |
*                                                                |___/
*
*
*                                                    ░
*                                                  ,▒░
*                                                 ¿░▓
*                                                ▒░▓╜
*                                              ,▒░▓╣`
*                                             ╓▒░▓╣░╓╜
*                                            ║▒▄▓╣╫▓▓
*                                           ╢▒▄▓▓▓╣╣
*                                         ,╣▒▓▓▓▓▓▀"
*                                            ▀▀"
*                                     ,╖╖╖╓,,
*                                q▒▒░░▒▒▒╨╜` ╓▒▐▓▓╣@╗╖
*                               ]▒▒▓██╜   ,∩░░░░▓╣╣╣▒▒▒╢N╖
*                               ▒░`     ╓╢▒▒▒░░░░▓▓╣╣╣▒▒▒▒╖
*                                 ,╓╗@╣▓███▓▓▓▓▄▄▄▓╣╢╢╣╣▒▒▒╣
*                              ║▒▓▓▒▒▒╢▓██▓▓▓▓▓▓▓▒▓██▄▒╢╣╣▒▒▒╗
*                              ╚╣╢▓▓▓▓▓▓████▓▓▓▀▒▒╢▓████▄▒╣╣▒▒╣,
*                                 `▀▀`   ▀████▒▒▒▒▒╢▓██████▄▄▄▄▄k
*                                         ▀█▀▒▒▒▄▒▒▒╢▓█████▓▓▓█`
*                                          ███▓▓▌▒▒▒▒╢▓███▓▓▓▀
*                                          █████▒▒▒▒▒▒╫▓█▓▓▀
*                                          ▓███▌▒▒▒▒▒▒╢╢▓▀
*                                          ▒███▒▒░░╣╣▓╜
*                                          ╢██▒▒æ▓▓"
*                                          ╢▓▓▓▀`
*                                          ╫▀
*
* ------------------------------------------------------------------------------------------
* Created by: Menno Bil
*
* Include this file in your project to make use of the RabBit engine.
* Every class, function and variable in this engine is inside the RB namespace.
*
* Command line options:
*	-assetPath "<path>"		->	(REQUIRED) Specify the asset locaion path
*	-renderDebug			->	Enables native graphics API validation (does not when RB_CONFIG_DIST is defined)
*/


// --Entry point--------------------
#include "RabBit/EntryPoint.h"
// ---------------------------------

#include "RabBit/utils/debug/Log.h"
#include "RabBit/utils/debug/Assert.h"

#include "RabBit/app/Application.h"

#include "RabBit/entity/Scene.h"
#include "RabBit/entity/GameObject.h"
#include "RabBit/entity/components/Mesh.h"
#include "RabBit/entity/components/Camera.h"
#include "RabBit/entity/components/Transform.h"

#include "RabBit/events/input/Input.h"
#include "RabBit/events/input/KeyCodes.h"
#include "RabBit/events/input/MouseCodes.h"

#include "RabBit/math/Vector.h"