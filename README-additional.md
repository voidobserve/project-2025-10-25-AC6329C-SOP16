- 进入流星灯声控模式时，有概率会有上一次流星灯动画的数据残留，导致显示异常，看起来像是流星灯卡住。直到有声控信号触发。
  刚进入流星灯动画时，应该清空颜色残留：
  ```c
    if (0 == _seg_rt->counter_mode_call)
    {
        // 刚进入该动画时，熄灭所有流星灯
        Adafruit_NeoPixel_fill(BLACK, _seg->start, _seg_len); // 全段填黑色，灭灯
    }
  ```
  
- 