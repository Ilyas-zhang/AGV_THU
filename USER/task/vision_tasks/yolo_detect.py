# ============================================================
# K210 / CanMV
# YOLOv2 路牌目标检测 + UART 控制
#
# 模型: det.kmodel (YOLOv2, 7 类, K210 SUPER 版)
# 类别:
#   0 = HORN         -> UART 'H'
#   1 = LEFT         -> UART 'L'
#   2 = PARK1        -> UART '1'
#   3 = PARK2        -> UART '2'
#   4 = RIGHT        -> UART 'R'
#   5 = SPEED_LIMIT  -> UART 'W'
#   6 = SPEED_RELEASE-> UART 'F'
#
# UART1:
#   TX = IO8
#   RX = IO6
#   115200 8N1
#   裸字符模式：直接发单字符，无帧协议，最低延时
#
# 模型加载方式 (二选一):
#   Flash: kpu.load_kmodel(0x300000, 581672)
#   SD卡:  kpu.load_kmodel('/sd/det.kmodel')
# ============================================================


import sensor
import image
import lcd
import time
import gc

from maix import KPU

from machine import UART
from fpioa_manager import fm


# ============================================================
# 1. 参数
# ============================================================

# ---- 模型 ----

# 从 SD 卡加载 (推荐，方便更换模型):
MODEL_PATH = '/sd/det.kmodel'

# 或从 Flash 加载 (需先用 kflash_gui 烧录 det.kmodel 到该地址):
# MODEL_ADDR   = 0x300000
# MODEL_SIZE   = 581672


# ---- YOLOv2 ----

LABELS = [
    "HORN",             # 0
    "LEFT",             # 1
    "PARK1",            # 2
    "PARK2",            # 3
    "RIGHT",            # 4
    "SPEED_LIMIT",      # 5
    "SPEED_RELEASE",    # 6
]

# Anchors (K210 SUPER anchor.txt 第 2 行, 归一化值)
ANCHOR = (
    1.67, 1.47,
    2.20, 2.09,
    2.97, 2.47,
    3.56, 3.13,
    5.53, 4.54,
)

YOLO_THRESHOLD = 0.6
YOLO_NMS       = 0.3

IMG_W = 320
IMG_H = 240
NET_W = 320
NET_H = 240
LAYER_W = 10
LAYER_H = 8


# ---- 稳定检测 ----

# 连续多少帧检测到同一类才发送命令
STABLE_FRAMES = 1

# 一个路牌离开多少帧以后允许再次触发
REARM_FRAMES = 3


# ---- UART 发送间隔 (ms) ----

SEND_INTERVAL_MS = 20


# ============================================================
# 2. 类别 → UART 命令映射 (裸字符)
# ============================================================

CLASS_COMMANDS = [
    "H",    # 0 = HORN
    "L",    # 1 = LEFT
    "1",    # 2 = PARK1
    "2",    # 3 = PARK2
    "R",    # 4 = RIGHT
    "W",    # 5 = SPEED_LIMIT
    "F",    # 6 = SPEED_RELEASE
]


# ============================================================
# 3. LCD
# ============================================================

lcd.init()

lcd.clear(lcd.RED)


# ============================================================
# 4. Camera
# ============================================================

sensor.reset()

sensor.set_pixformat(sensor.RGB565)

sensor.set_framesize(sensor.QVGA)

sensor.set_vflip(True)       # 翻转摄像头

sensor.set_hmirror(False)    # 关闭水平镜像

sensor.skip_frames(time=1000)

clock = time.clock()


# ============================================================
# 5. UART1
#
# TX = IO8
# RX = IO6
# ============================================================

fm.register(8, fm.fpioa.UART1_TX, force=True)

fm.register(6, fm.fpioa.UART1_RX, force=True)

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
print("YOLO DETECT + UART (RAW)")
print("UART1 115200 8N1")
print("TX=IO8  RX=IO6")
print("================================")
print("")


# ============================================================
# 6. KPU
# ============================================================

kpu = KPU()

print("Loading model...")

# 从 SD 卡加载
kpu.load_kmodel(MODEL_PATH)

# 或从 Flash 加载:
# kpu.load_kmodel(MODEL_ADDR, MODEL_SIZE)

print("Model loaded!")

kpu.init_yolo2(
    ANCHOR,
    anchor_num=(int)(len(ANCHOR) / 2),
    img_w=IMG_W,
    img_h=IMG_H,
    net_w=NET_W,
    net_h=NET_H,
    layer_w=LAYER_W,
    layer_h=LAYER_H,
    threshold=YOLO_THRESHOLD,
    nms_value=YOLO_NMS,
    classes=len(LABELS),
)


# ============================================================
# 7. UART 发送 (裸字符)
# ============================================================

def send_command(class_index):

    command = CLASS_COMMANDS[class_index]

    name = LABELS[class_index]

    uart_A.write(command)

    print("[%s] -> %s" % (name, command))


# ============================================================
# 8. STM32 返回数据 (帧协议解析, 仅用于心跳)
# ============================================================

RX_BUF_SIZE = 64

rx_buf = bytearray(RX_BUF_SIZE)

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
            rx_index = 0
            rx_flag = True

        elif rx_flag and ch == '#':
            payload = rx_buf[:rx_index].decode()
            rx_flag = False
            last_rx_msg = payload

            if payload == "alive":
                print("[STM32 HEARTBEAT]")
            else:
                print("[STM32 RX]", payload)

        elif rx_flag:
            if rx_index < RX_BUF_SIZE:
                rx_buf[rx_index] = ord(ch)
                rx_index += 1
            else:
                rx_flag = False
                rx_index = 0


# ============================================================
# 9. 防止重复命令
# ============================================================

candidate_class = -1

candidate_count = 0

sign_locked = False

unknown_count = 0


# ============================================================
# 10. 启动画面
# ============================================================

print("")
print("================================")
print("YOLOv2 ROAD SIGN DETECTION")
print("")
for i, name in enumerate(LABELS):
    print("  Class %d = %s" % (i, name))
print("")
print("7 classes, YOLOv2")
print("Threshold: %.2f" % YOLO_THRESHOLD)
print("NMS: %.2f" % YOLO_NMS)
print("Stable: %d frames" % STABLE_FRAMES)
print("Rearm: %d frames" % REARM_FRAMES)
print("Send interval: %d ms" % SEND_INTERVAL_MS)
print("")
print("================================")
print("")


# ============================================================
# 11. 主循环
# ============================================================

last_send = time.ticks_ms()

while True:

    gc.collect()

    clock.tick()

    img = sensor.snapshot()

    # ---- KPU 推理 ----

    kpu.run_with_output(img)

    dect = kpu.regionlayer_yolo2()

    fps = clock.fps()

    # ---- 处理检测结果 ----

    if len(dect) > 0:

        # 取置信度最高的检测
        best_det = max(dect, key=lambda d: d[5])

        best_class = best_det[4]
        best_score = best_det[5]

        # 画检测框
        img.draw_rectangle(
            best_det[0], best_det[1],
            best_det[2], best_det[3],
            color=(0, 255, 0)
        )

        info = "%s %.2f" % (LABELS[best_class], best_score)
        img.draw_string(
            best_det[0], best_det[1],
            info,
            color=(255, 0, 0),
            scale=2.0
        )

        # ---- 稳定检测逻辑 ----

        if best_class == candidate_class:
            candidate_count += 1
        else:
            candidate_class = best_class
            candidate_count = 1

        unknown_count = 0

        now = time.ticks_ms()

        if (
            candidate_count >= STABLE_FRAMES
            and not sign_locked
            and now - last_send >= SEND_INTERVAL_MS
        ):

            send_command(best_class)

            sign_locked = True

            last_send = now

    else:

        # 未检测到路牌
        candidate_class = -1
        candidate_count = 0

        unknown_count += 1

        if unknown_count >= REARM_FRAMES:

            if sign_locked:
                print("READY")

            sign_locked = False

            unknown_count = REARM_FRAMES

    # ---- FPS ----

    img.draw_string(
        0, 0,
        "%2.1ffps" % fps,
        color=(0, 60, 255),
        scale=2.0
    )

    # ---- LCD 显示 STM32 消息 ----

    if last_rx_msg:

        img.draw_string(
            0, 25,
            "RX:%s" % last_rx_msg[:8],
            color=(255, 128, 0),
            scale=1.5
        )

    lcd.display(img)

    # ---- 检查 STM32 心跳 ----

    check_uart_rx()
