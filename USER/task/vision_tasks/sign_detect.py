# ============================================================
# K210 / CanMV
# 自主学习路牌识别 + UART 控制
#
# 修正版：
# 1. 修复 BOOT 无法拍照问题
# 2. BOOT 自动寻找 + IO16 fallback
# 3. 按键采用“按下-释放”检测，避免漏按
# 4. LCD 始终显示完整 224x224
# 5. KPU 内部隐式使用 160x160 ROI
# 6. 不使用 copy() / resize()，降低内存占用
#
# Class 1 = LEFT        -> UART "$L#"
# Class 2 = RIGHT       -> UART "$R#"
# Class 3 = HORN        -> UART "$H#"
# Class 4 = SLOW        -> UART "$W#"
# Class 5 = FAST        -> UART "$F#"
# Class 6 = RED_LIGHT   -> UART "$D#"
# Class 7 = YELLOW_LIGHT-> UART "$Y#"
# Class 8 = GREEN_LIGHT -> UART "$G#"
# Class 9 = BACK_IN     -> UART "$B#"
#
# UART1:
# TX = IO8
# RX = IO6
# 115200 8N1
# 帧协议：$payload#（与 STM32 端 k210_comm.c 一致）
#   K210 发送 "$L#"/"$R#"/"$H#"/"$W#"/"$F#"/"$D#"/"$Y#"/"$G#"/"$B#"
#   STM32 发送 "$alive#" → 心跳
#
# 模型:
# /sd/KPU/self_learn_classifier/mb-0.25.kmodel
# ============================================================


import sensor
import image
import lcd
import time
import gc

from maix import KPU
from maix import GPIO

from machine import UART
from fpioa_manager import fm

# board_info 这次保留
# 但不再强制要求必须存在 BOOT_KEY
try:
    from board import board_info
except:
    board_info = None


# ============================================================
# 1. 基本参数
# ============================================================

CLASS_NUM = 9

PIC_PER_CLASS = 10


# 识别阈值
THRESHOLD = 98.5


# 连续识别多少帧才真正发送命令
STABLE_FRAMES = 3


# 一个路牌离开多少帧以后允许再次触发
REARM_FRAMES = 8


# ============================================================
# 2. 隐式 ROI
#
# LCD:
#     仍显示完整 224 x 224
#
# KPU:
#     中央 160 x 160 为有效真实画面
#     外围固定灰色
# ============================================================

ROI_X = 32
ROI_Y = 32

ROI_W = 160
ROI_H = 160


MASK_COLOR = (
    128,
    128,
    128
)


# ============================================================
# 3. 类别
# ============================================================

CLASS_NAMES = [
    "LEFT",
    "RIGHT",
    "HORN",
    "SLOW",
    "FAST",
    "RED",
    "YELLOW",
    "GREEN",
    "BACK"
]


CLASS_COMMANDS = [
    "$L#",
    "$R#",
    "$H#",
    "$W#",
    "$F#",
    "$D#",
    "$Y#",
    "$G#",
    "$B#"
]


# ============================================================
# 4. 训练特征
# ============================================================

features = [
    [],
    [],
    []
]


# ============================================================
# 5. 状态
#
# 0 = INIT
# 1 = TRAIN
# 2 = CLASSIFY
# ============================================================

state = 0

current_class = 0

current_pic = 0


# ============================================================
# 6. LCD
# ============================================================

lcd.init()


# ============================================================
# 7. CAMERA
# ============================================================

sensor.reset()

sensor.set_pixformat(
    sensor.RGB565
)

sensor.set_framesize(
    sensor.QVGA
)

sensor.set_windowing(
    (224, 224)
)

sensor.skip_frames(
    time=1200
)

clock = time.clock()


# ============================================================
# 8. UART1
#
# TX = IO8
# RX = IO6
# ============================================================

fm.register(
    8,
    fm.fpioa.UART1_TX,
    force=True
)

fm.register(
    6,
    fm.fpioa.UART1_RX,
    force=True
)


uart_A = UART(
    UART.UART1,
    115200,
    8,
    0,
    1,
    timeout=100,
    read_buf_len=4096
)


print("")
print("================================")
print("UART READY")
print("UART1 115200 8N1")
print("TX = IO8")
print("RX = IO6")
print("================================")
print("")


# ============================================================
# 9. BOOT 按键初始化
#
# 自动寻找固件里可能存在的 BOOT 定义。
#
# 如果都不存在：
# fallback -> IO16
# ============================================================

BOOT_PIN = None


if board_info is not None:

    try:
        BOOT_PIN = board_info.BOOT_KEY
        print("BOOT source = board_info.BOOT_KEY")
    except:
        pass


if BOOT_PIN is None and board_info is not None:

    try:
        BOOT_PIN = board_info.BOOT
        print("BOOT source = board_info.BOOT")
    except:
        pass


if BOOT_PIN is None and board_info is not None:

    try:
        BOOT_PIN = board_info.KEY
        print("BOOT source = board_info.KEY")
    except:
        pass


# ------------------------------------------------------------
# 如果当前固件没有任何 BOOT 定义
# 使用标准 IO16
# ------------------------------------------------------------

if BOOT_PIN is None:

    BOOT_PIN = 16

    print(
        "BOOT source = fallback IO16"
    )


print(
    "BOOT PIN =",
    BOOT_PIN
)


# ------------------------------------------------------------
# BOOT -> GPIOHS0
# ------------------------------------------------------------

fm.register(
    BOOT_PIN,
    fm.fpioa.GPIOHS0,
    force=True
)


# ------------------------------------------------------------
# 优先尝试输入上拉
# 某些固件 GPIO 构造参数可能不同
# 所以增加兼容 fallback
# ------------------------------------------------------------

try:

    boot_gpio = GPIO(
        GPIO.GPIOHS0,
        GPIO.IN,
        GPIO.PULL_UP
    )

    print(
        "BOOT GPIO = INPUT PULL_UP"
    )

except:

    boot_gpio = GPIO(
        GPIO.GPIOHS0,
        GPIO.IN
    )

    print(
        "BOOT GPIO = INPUT"
    )


print("")
print("BOOT KEY READY")
print("")


# ============================================================
# 10. BOOT 按键检测
#
# 这里不再使用旧版 last_key 下降沿方法。
#
# 新逻辑：
#
#   检测到 LOW
#       ↓
#   消抖
#       ↓
#   等待用户松开
#       ↓
#   返回 True
#
# 每次实体按键只会触发一次。
# ============================================================

def boot_pressed():

    # BOOT正常情况下：
    #
    # 未按 = 1
    # 按下 = 0

    if boot_gpio.value() != 0:

        return False


    # --------------------------------------------------------
    # 第一次检测为低
    # 消抖
    # --------------------------------------------------------

    time.sleep_ms(
        30
    )


    # --------------------------------------------------------
    # 消抖后不是低电平
    # 认为是干扰
    # --------------------------------------------------------

    if boot_gpio.value() != 0:

        return False


    print(
        "BOOT PRESSED"
    )


    # --------------------------------------------------------
    # 等待用户松开
    #
    # 这样一次按压不会触发多张照片
    # --------------------------------------------------------

    while boot_gpio.value() == 0:

        time.sleep_ms(
            10
        )


    # --------------------------------------------------------
    # 松开后再次消抖
    # --------------------------------------------------------

    time.sleep_ms(
        50
    )


    return True


# ============================================================
# 11. KPU
# ============================================================

kpu = KPU()


MODEL_PATH = \
    "/sd/KPU/self_learn_classifier/mb-0.25.kmodel"


print(
    "Loading model..."
)


kpu.load_kmodel(
    MODEL_PATH
)


print(
    "Model loaded!"
)


# ============================================================
# 12. UART发送
# ============================================================

def send_command(class_index):

    command = \
        CLASS_COMMANDS[
            class_index
        ]

    name = \
        CLASS_NAMES[
            class_index
        ]


    uart_A.write(
        command
    )


    print("")
    print("================================")

    print(
        "ROAD SIGN DETECTED:",
        name
    )

    print(
        "SEND ->",
        command
    )

    print("================================")
    print("")


# ============================================================
# 13. STM32 返回数据（$payload# 帧协议解析）
#
# 与 STM32 端 k210_comm.c 的帧解析器对称：
#   收到 '$' → 开始新帧
#   收到 '#' → 帧结束，处理 payload
#   其他字符 → 存入缓冲
#
# STM32 端 test_k210_comm.c 会发送：
#   "$alive#"   — 心跳帧（每 1 秒）
#   "$echo:xx#" — 回显帧（K210 发送后原样回传）
# ============================================================

RX_BUF_SIZE = 64

rx_buf = bytearray(
    RX_BUF_SIZE
)

rx_index = 0

rx_flag = False

last_rx_msg = ""


def check_uart_rx():

    global rx_index
    global rx_flag
    global last_rx_msg

    if not uart_A.any():

        return

    data = uart_A.read()

    if not data:

        return

    try:

        text = data.decode()

    except:

        return

    for ch in text:

        if ch == '$':

            # 帧起始
            rx_index = 0
            rx_flag = True

        elif rx_flag and ch == '#':

            # 帧结束
            payload = \
                rx_buf[:rx_index].decode()

            rx_flag = False

            last_rx_msg = payload

            # ---- 处理 payload ----

            if payload == "alive":

                print(
                    "[STM32 HEARTBEAT]"
                )

            elif payload.startswith(
                "echo:"
            ):

                echo_content = \
                    payload[5:]

                print(
                    "[STM32 ECHO]",
                    echo_content
                )

            else:

                print(
                    "[STM32 RX]",
                    payload
                )

        elif rx_flag:

            # 帧内数据
            if rx_index < RX_BUF_SIZE:

                rx_buf[rx_index] = \
                    ord(ch)

                rx_index += 1

            else:

                # 缓冲溢出，丢弃
                rx_flag = False
                rx_index = 0


# ============================================================
# 14. 隐式 ROI
#
# 只修改送给 KPU 的图像。
#
# 不用于 LCD。
#
# 不创建额外 Image。
# ============================================================

def mask_background(img):


    # 上

    if ROI_Y > 0:

        img.draw_rectangle(
            0,
            0,
            224,
            ROI_Y,
            color=MASK_COLOR,
            fill=True
        )


    # 下

    bottom_h = \
        224 - ROI_Y - ROI_H

    if bottom_h > 0:

        img.draw_rectangle(
            0,
            ROI_Y + ROI_H,
            224,
            bottom_h,
            color=MASK_COLOR,
            fill=True
        )


    # 左

    if ROI_X > 0:

        img.draw_rectangle(
            0,
            ROI_Y,
            ROI_X,
            ROI_H,
            color=MASK_COLOR,
            fill=True
        )


    # 右

    right_w = \
        224 - ROI_X - ROI_W

    if right_w > 0:

        img.draw_rectangle(
            ROI_X + ROI_W,
            ROI_Y,
            right_w,
            ROI_H,
            color=MASK_COLOR,
            fill=True
        )


# ============================================================
# 15. 防止重复命令
# ============================================================

candidate_class = -1

candidate_count = 0

sign_locked = False

unknown_count = 0


# ============================================================
# 16. 启动画面终端信息
# ============================================================

print("")
print("================================")
print("ROAD SIGN SELF LEARNING")
print("")
print("Class 1 = RIGHT")
print("Class 2 = LEFT")
print("Class 3 = STOP")
print("")
print("5 pictures / class")
print("")
print("BOOT = capture")
print("")
print("IMPORTANT:")
print("Run this code as main.py")
print("Do NOT rely on IDE Run")
print("")
print("Press BOOT to start")
print("================================")
print("")


# ============================================================
# 17. 主循环
# ============================================================

while True:


    gc.collect()

    clock.tick()


    # ========================================================
    # STATE 0
    # 等待开始
    # ========================================================

    if state == 0:


        img = sensor.snapshot()


        img.draw_string(
            5,
            20,
            "ROAD SIGN",
            color=(255, 0, 0),
            scale=2
        )


        img.draw_string(
            5,
            55,
            "SELF LEARNING",
            color=(255, 0, 0),
            scale=2
        )


        img.draw_string(
            5,
            100,
            "C1 RIGHT",
            color=(0, 255, 0),
            scale=1.5
        )


        img.draw_string(
            5,
            125,
            "C2 LEFT",
            color=(0, 255, 0),
            scale=1.5
        )


        img.draw_string(
            5,
            150,
            "C3 STOP",
            color=(0, 255, 0),
            scale=1.5
        )


        img.draw_string(
            5,
            190,
            "PRESS BOOT",
            color=(255, 255, 0),
            scale=1.5
        )


        # ----------------------------------------------------
        # 按一次 BOOT
        # 进入训练
        # ----------------------------------------------------

        if boot_pressed():

            state = 1

            current_class = 0

            current_pic = 0


            print("")
            print("================================")
            print("TRAIN MODE")
            print("")
            print(
                "Class 1 =",
                CLASS_NAMES[0]
            )
            print("")
            print(
                "Press BOOT to capture Pic 1"
            )
            print("================================")
            print("")


            # 防止状态切换过快

            time.sleep_ms(
                200
            )


        img.draw_string(
            5,
            215,
            "FPS %.1f"
            % clock.fps(),
            color=(0, 255, 255),
            scale=1
        )


        check_uart_rx()


        lcd.display(
            img
        )


    # ========================================================
    # STATE 1
    # TRAIN
    # ========================================================

    elif state == 1:


        name = \
            CLASS_NAMES[
                current_class
            ]


        # ----------------------------------------------------
        # 正常完整 LCD 图像
        # ----------------------------------------------------

        img = sensor.snapshot()


        img.draw_string(
            5,
            5,
            "TRAIN",
            color=(255, 0, 0),
            scale=1.5
        )


        img.draw_string(
            5,
            30,
            "C%d %s"
            % (
                current_class + 1,
                name
            ),
            color=(0, 255, 0),
            scale=1.5
        )


        img.draw_string(
            5,
            55,
            "PIC %d / %d"
            % (
                current_pic + 1,
                PIC_PER_CLASS
            ),
            color=(0, 255, 255),
            scale=1.5
        )


        img.draw_string(
            5,
            195,
            "PRESS BOOT CAPTURE",
            color=(255, 255, 0),
            scale=1
        )


        # ----------------------------------------------------
        # 按 BOOT 拍一张
        # ----------------------------------------------------

        if boot_pressed():


            print("")
            print(
                "CAPTURE:",
                name,
                current_pic + 1,
                "/",
                PIC_PER_CLASS
            )


            # ------------------------------------------------
            # BOOT松开后
            # 再稍微等一下
            #
            # 防止按键时手碰摄像头产生抖动
            # ------------------------------------------------

            time.sleep_ms(
                100
            )


            # ------------------------------------------------
            # KPU 专用训练帧
            #
            # 用户不会看到遮罩
            # ------------------------------------------------

            train_img = \
                sensor.snapshot()


            mask_background(
                train_img
            )


            train_img.pix_to_ai()


            feature = \
                kpu.run_with_output(
                    train_img,
                    get_feature=True
                )


            features[
                current_class
            ].append(
                feature
            )


            current_pic += 1


            print(
                "CAPTURE OK"
            )


            print(
                "Saved pictures:",
                current_pic
            )


            # ------------------------------------------------
            # 当前类别拍满
            # ------------------------------------------------

            if current_pic >= \
                    PIC_PER_CLASS:


                print("")
                print(
                    "Class",
                    current_class + 1,
                    name,
                    "FINISHED"
                )


                current_class += 1

                current_pic = 0


                # ============================================
                # 3 类全部完成
                # ============================================

                if current_class >= \
                        CLASS_NUM:


                    state = 2


                    candidate_class = -1

                    candidate_count = 0

                    sign_locked = False

                    unknown_count = 0


                    print("")
                    print("================================")
                    print("ALL TRAINING FINISHED")
                    print("")
                    print(
                        "START CLASSIFICATION"
                    )
                    print("================================")
                    print("")


                    time.sleep_ms(
                        500
                    )


                # ============================================
                # 下一类别
                # ============================================

                else:


                    print("")
                    print(
                        "NEXT CLASS:"
                    )

                    print(
                        "Class",
                        current_class + 1,
                        "=",
                        CLASS_NAMES[
                            current_class
                        ]
                    )

                    print(
                        "Press BOOT for Pic 1"
                    )
                    print("")


                    time.sleep_ms(
                        300
                    )


            else:


                print(
                    "Next:",
                    name,
                    "Pic",
                    current_pic + 1
                )

                print("")


        # ----------------------------------------------------
        # LCD 显示完整摄像头
        # ----------------------------------------------------

        img.draw_string(
            5,
            215,
            "FPS %.1f"
            % clock.fps(),
            color=(0, 255, 255),
            scale=1
        )


        check_uart_rx()


        lcd.display(
            img
        )


    # ========================================================
    # STATE 2
    # CLASSIFY
    # ========================================================

    elif state == 2:


        # ----------------------------------------------------
        # KPU 专用帧
        #
        # 用户不会看到这个被遮罩的图
        # ----------------------------------------------------

        ai_img = \
            sensor.snapshot()


        mask_background(
            ai_img
        )


        ai_img.pix_to_ai()


        current_feature = \
            kpu.run_with_output(
                ai_img,
                get_feature=True
            )


        # ----------------------------------------------------
        # 最佳类别
        # ----------------------------------------------------

        best_score = 0

        best_class = -1


        for class_index in \
                range(CLASS_NUM):


            for train_feature in \
                    features[
                        class_index
                    ]:


                score = \
                    kpu.feature_compare(
                        train_feature,
                        current_feature
                    )


                if score > \
                        best_score:


                    best_score = \
                        score

                    best_class = \
                        class_index


        # ====================================================
        # 可靠识别
        # ====================================================

        if (
            best_class >= 0
            and
            best_score > THRESHOLD
        ):


            if best_class == \
                    candidate_class:


                candidate_count += 1


            else:


                candidate_class = \
                    best_class

                candidate_count = 1


            unknown_count = 0


            # ------------------------------------------------
            # 连续 STABLE_FRAMES
            # ------------------------------------------------

            if (
                candidate_count
                >= STABLE_FRAMES
                and
                not sign_locked
            ):


                print(
                    "CONFIRMED:",
                    CLASS_NAMES[
                        best_class
                    ],
                    "Score:",
                    best_score
                )


                send_command(
                    best_class
                )


                sign_locked = True


        # ====================================================
        # 未可靠识别
        # ====================================================

        else:


            candidate_class = -1

            candidate_count = 0


            unknown_count += 1


            if unknown_count >= \
                    REARM_FRAMES:


                if sign_locked:

                    print(
                        "READY FOR NEXT SIGN"
                    )


                sign_locked = False

                unknown_count = \
                    REARM_FRAMES


        # ----------------------------------------------------
        # LCD 专用完整图像
        # ----------------------------------------------------

        display_img = \
            sensor.snapshot()


        # ----------------------------------------------------
        # 结果显示
        # ----------------------------------------------------

        if (
            best_class >= 0
            and
            best_score > THRESHOLD
        ):


            display_img.draw_string(
                5,
                185,
                "%s %.1f"
                % (
                    CLASS_NAMES[
                        best_class
                    ],
                    best_score
                ),
                color=(0, 255, 0),
                scale=1.5
            )


        else:


            display_img.draw_string(
                5,
                185,
                "NO SIGN %.1f"
                % best_score,
                color=(255, 0, 0),
                scale=1.2
            )


        display_img.draw_string(
            5,
            215,
            "FPS %.1f"
            % clock.fps(),
            color=(0, 255, 255),
            scale=1
        )


        check_uart_rx()


        # ----------------------------------------------------
        # LCD 显示最近收到的 STM32 消息
        # ----------------------------------------------------

        if last_rx_msg:

            display_img.draw_string(
                5,
                205,
                "RX:%s"
                % last_rx_msg[:8],
                color=(255, 128, 0),
                scale=1
            )


        lcd.display(
            display_img
        )
