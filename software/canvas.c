#include "common.h"
#include "Ai.h"
#include "canvas.h"

extern uint8_t xdata input_image[IMAGE_WIDTH * IMAGE_HEIGHT];
uint8_t xdata canvas[CANVAS_HEIGHT * CANVAS_WIDTH] = {0};

void clean_canvas()
{
    memset(canvas, 0, CANVAS_HEIGHT * CANVAS_WIDTH * sizeof(canvas[0]));
}

// 给出一个字符在canvas中起始列，分割这个字符,居中放入input_image，保证四边空白部分等宽
bool canvas_process_character(uint32_t start_col, uint32_t end_col)
{
    uint32_t col, row;
    uint32_t top_row, bottom_row;
    uint32_t hor_offset, ver_offset;
    uint32_t effective_height;

    // 处理宽度只有一列的情况，区分点和有效字符
    if (end_col - start_col <= 3)
    {
        uint32_t count = 0;
        for (col = start_col; col <= end_col; col++)
        {
            for (row = 0; row < CANVAS_HEIGHT; row++)
            {
                if (canvas[row * CANVAS_WIDTH + col] > 0)
                {
                    count++;
                }
            }
        }

        if (count < 10)
        {
            return false; // 认为是干扰点，跳过
        }
    }

    clean_input_image();

    // 计算水平偏移量
    hor_offset = (CHAR_IMG_SIZE - (end_col - start_col + 1)) / 2;

    // 计算有效的字符高度范围
    top_row = CANVAS_HEIGHT;
    bottom_row = 0;
    for (row = 0; row < CANVAS_HEIGHT; row++)
    {
        for (col = start_col; col <= end_col; col++)
        {
            if (canvas[row * CANVAS_WIDTH + col] > 0)
            {
                if (row < top_row)
                    top_row = row;
                if (row > bottom_row)
                    bottom_row = row;
            }
        }
    }

    // 计算有效字符区域的高度
    effective_height = bottom_row - top_row + 1;

    // 计算垂直偏移量（基于有效高度居中）
    ver_offset = (CHAR_IMG_SIZE - effective_height) / 2;

    // 分割字符区域并扩展到 28x28 大小，居中处理
    for (col = start_col; col <= end_col; col++)
    {
        for (row = 0; row < CANVAS_HEIGHT; row++)
        {
            if (canvas[row * CANVAS_WIDTH + col] > 0)
            {
                // 将字符区域扩展到 28x28 大小并居中
                uint32_t new_col = col - start_col + hor_offset;
                uint32_t new_row = row - top_row + ver_offset;

                // 如果字符高度和位置不适合居中，进行调整
                if (new_row < 0)
                {
                    new_row = 0;
                }
                else if (new_row >= CHAR_IMG_SIZE)
                {
                    new_row = CHAR_IMG_SIZE - 1;
                }

                input_image[new_row * CHAR_IMG_SIZE + new_col] = canvas[row * CANVAS_WIDTH + col];
            }
        }
    }
#if COLLECT_MODE
    {
        int xxx;
        for (xxx = 0; xxx < IMAGE_WIDTH * IMAGE_HEIGHT; xxx++)
        {
            printf("%3d,", input_image[xxx]);
            if (xxx % IMAGE_WIDTH == IMAGE_WIDTH - 1)
                printf("\r\n");
        }
        printf("@\r\n");
    }
#endif
    return true;
}