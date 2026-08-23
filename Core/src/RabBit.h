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
*   -assetPath <path>       ->  (REQUIRED) Specify the asset locaion path
*   -renderDebug            ->  Enables native graphics API validation
*   -pix                    ->  (Windows only) Loads the PIX runtime DLL so that PIX can be attatched after startup for GPU debugging
*/


// Define on the app side in the file with "RB::CreateApplication"
#ifdef RB_DEFINE_ENTRY_POINT
    #include "rabbit/EntryPoint.h"
#endif

#include "rabbit/utils/debug/Log.h"
#include "rabbit/utils/debug/Assert.h"

#include "rabbit/app/Application.h"
#include "rabbit/app/PlatformService.h"

#include "rabbit/events/ApplicationEvent.h"
#include "rabbit/events/KeyEvent.h"
#include "rabbit/events/MouseEvent.h"
#include "rabbit/events/WindowEvent.h"

#include "rabbit/events/input/Input.h"
#include "rabbit/events/input/KeyCodes.h"
#include "rabbit/events/input/MouseCodes.h"

#include "rabbit/math/Misc.h"
#include "rabbit/math/Matrix.h"
#include "rabbit/math/Vector.h"

#include "rabbit/entity/Scene.h"
#include "rabbit/entity/GameObject.h"
#include "rabbit/entity/components/Transform.h"
#include "rabbit/entity/components/Camera.h"
#include "rabbit/entity/components/Mesh.h"
#include "rabbit/entity/components/UI.h"
#include "rabbit/entity/components/Light.h"
#include "rabbit/entity/components/NetworkTransformSync.h"
#include "rabbit/entity/components/ScreenCapturer.h"

#include "rabbit/graphics/passes/GBuffer.h"
#include "rabbit/graphics/passes/CascadedShadow.h"
#include "rabbit/graphics/passes/DeferredLighting.h"
#include "rabbit/graphics/passes/Overlay2D.h"
#include "rabbit/graphics/passes/Smaa.h"
#include "rabbit/graphics/passes/ToneMapping.h"
#include "rabbit/graphics/passes/ScreenCapturePass.h"