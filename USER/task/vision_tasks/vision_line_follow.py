# ============================================================
# K210 / CanMV
# 视觉循迹 — 摄像头找黑线质心，UART 发误差给 STM32
#
# 帧格式: $e<value>#
#   value: -100 ~ +100  (负=线偏左, 正=线偏右, 0=居中)
#   value: 999          (脱线，未找到黑线)
#
# UART1: TX=IO8, RX=IO6, 115200 8N1
#
# 摄像头: QQVGA 160×120 灰度
# 搜索区域: 画面下方 60%~90% (刚过车头的线)
# ============================================================


import sensor
import image
import lcd
import time
import gc

from machine import UART
from fpioa_manager import fm


# ============================================================
# 1. 参数
# ============================================================

IMG_W = 160
IMG_H = 120

# 暗像素阈值 (L 通道, 0~255)
LINE_THRESH = [
    (0, 60)
]

# 搜索区域: 画面下方 70%~95%
# 摄像头盲区 20cm, 画面底部≈20cm处路面, 越往上越远
# 循迹需要看近处线位置, 所以 ROI 尽量靠下
ROI_Y0 = 84
ROI_Y1 = 114
ROI_H = ROI_Y1 - ROI_Y0

# 误差归一化范围
ERROR_MAX = 100

# 发送间隔 (ms)
SEND_INTERVAL_MS = 50

# 脱线标志
NO_LINE = 999

# blob 最小像素数 (过滤噪声)
PIXELS_THRESH = 20
AREA_THRESH = 20


# ============================================================
# 2. UART1
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

uart = UART(
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
print("VISION LINE FOLLOW")
print("UART1 115200 8N1")
print("TX=IO8  RX=IO6")
print("================================")
print("")


# ============================================================
# 3. Camera
# ============================================================

sensor.reset()

sensor.set_pixformat(
    sensor.GRAYSCALE
)

sensor.set_framesize(
    sensor.QQVGA
)

sensor.skip_frames(
    time=2000
)

sensor.set_auto_gain(
    False
)

sensor.set_auto_exposure(
    False
)


# ============================================================
# 4. LCD
# ============================================================

lcd.init()


# ============================================================
# 5. 状态
# ============================================================

center_x = IMG_W // 2

clock = time.clock()

last_send = time.ticks_ms()


# ============================================================
# 6. 发送
# ============================================================

def send_error(err):

    msg = \
        "$e{}#".format(err)

    uart.write(msg)


# ============================================================
# 7. 接收 STM32 心跳 (可选)
# ============================================================

def check_uart_rx():

    if not uart.any():
        return

    data = uart.read()

    if not data:
        return

    try:
        text = data.decode()
    except:
        return

    if "alive" in text:
        print(
            "[STM32 HEARTBEAT]"
        )


# ============================================================
# 8. 主循环
# ============================================================

while True:

    clock.tick()

    gc.collect()

    img = sensor.snapshot()

    # ---- 在下方区域寻找暗色 blob ----

    blobs = img.find_blobs(
        LINE_THRESH,
        roi=(
            0,
            ROI_Y0,
            IMG_W,
            ROI_H
        ),
        pixels_threshold=PIXELS_THRESH,
        area_threshold=AREA_THRESH,
        merge=True
    )

    now = time.ticks_ms()

    if now - last_send < SEND_INTERVAL_MS:

        lcd.display(img)

        check_uart_rx()

        continue

    last_send = now

    # ---- 有线 ----

    if blobs:

        best = max(
            blobs,
            key=lambda b: b.pixels()
        )

        cx = best.cx()

        # 画十字标记质心

        img.draw_cross(
            cx,
            best.cy(),
            color=255
        )

        # 画 ROI 框

        img.draw_rectangle(
            0,
            ROI_Y0,
            IMG_W,
            ROI_H,
            color=128,
            fill=False
        )

        # 计算误差: 负=偏左, 正=偏右

        err = int(
            (cx - center_x)
            * ERROR_MAX
            / center_x
        )

        err = max(
            -ERROR_MAX,
            min(
                ERROR_MAX,
                err
            )
        )

        send_error(err)

        img.draw_string(
            5,
            5,
            "E:%d"
            % err,
            color=255,
            scale=1
        )

    # ---- 脱线 ----

    else:

        send_error(NO_LINE)

        img.draw_rectangle(
            0,
            ROI_Y0,
            IMG_W,
            ROI_H,
            color=128,
            fill=False
        )

        img.draw_string(
            5,
            5,
            "NO LINE",
            color=255,
            scale=1
        )

    # ---- FPS ----

    img.draw_string(
        5,
        IMG_H - 15,
        "FPS:%.1f"
        % clock.fps(),
        color=200,
        scale=1
    )

    lcd.display(img)

    check_uart_rx()
