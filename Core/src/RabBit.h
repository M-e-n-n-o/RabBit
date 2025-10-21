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
*	-assetPath <path>		->	(REQUIRED) Specify the asset locaion path
*	-renderDebug			->	Enables native graphics API validation (does not when RB_CONFIG_DIST is defined)
*/


// --Entry point--------------------
#include "rabbit/EntryPoint.h"
// ---------------------------------

#include "rabbit/utils/debug/Log.h"
#include "rabbit/utils/debug/Assert.h"

#include "rabbit/app/Application.h"

#include "rabbit/entity/Scene.h"
#include "rabbit/entity/GameObject.h"
#include "rabbit/entity/components/Transform.h"
#include "rabbit/entity/components/Camera.h"
#include "rabbit/entity/components/Mesh.h"
#include "rabbit/entity/components/Rect2D.h"

#include "rabbit/events/input/Input.h"
#include "rabbit/events/input/KeyCodes.h"
#include "rabbit/events/input/MouseCodes.h"

#include "rabbit/math/Misc.h"
#include "rabbit/math/Matrix.h"
#include "rabbit/math/Vector.h"