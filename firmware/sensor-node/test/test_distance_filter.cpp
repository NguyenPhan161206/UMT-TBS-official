// test_distance_filter.cpp — Host unit test cho DistanceFilter (cluster-EMA).
// Kiểm chứng thuật toán giữ nguyên hành vi từ bản gốc: warmup, init, OK,
// HOLD_JUMP khi bước nhảy chưa xác nhận, ACCEPT_JUMP sau xác nhận.
// Chạy bởi test_runner.cpp (main duy nhất).
#include <string.h>

#include <unity.h>

#include "distance_filter.h"

// Feed M mẫu ổn định quanh 100cm -> sau >=5 mẫu phải INIT rồi OK.
void test_filter_stable_readings(void)
{
    DistanceFilter f;
    f.reset();

    bool gotOutput = false;
    float output = 0.0f;
    for (int i = 0; i < 10; ++i)
    {
        FilterResult r = f.process(100.0f + ((i % 2) ? 1.0f : -1.0f));
        if (r.hasOutput)
        {
            gotOutput = true;
            output = r.outputCm;
        }
    }

    TEST_ASSERT_TRUE_MESSAGE(gotOutput, "Phải có output sau đủ mẫu");
    TEST_ASSERT_FLOAT_WITHIN(5.0f, 100.0f, output);

    float stable = 0.0f;
    TEST_ASSERT_TRUE(f.getStable(stable));
    TEST_ASSERT_FLOAT_WITHIN(5.0f, 100.0f, stable);
}

// Không đủ mẫu -> chưa có output.
void test_filter_warmup(void)
{
    DistanceFilter f;
    f.reset();

    for (int i = 0; i < 4; ++i)
    {
        FilterResult r = f.process(50.0f);
        TEST_ASSERT_FALSE(r.hasOutput);
        TEST_ASSERT_EQUAL_STRING("WARMUP", r.status);
    }
}

// Nhiễu không tạo cụm đủ mạnh -> output giữ giá trị cũ (100).
void test_filter_noise_keeps_old(void)
{
    DistanceFilter f;
    f.reset();

    for (int i = 0; i < 8; ++i)
    {
        f.process(100.0f);
    }
    float old = 0.0f;
    TEST_ASSERT_TRUE(f.getStable(old));

    // Dùng nhiễu nằm trong khoảng < 30cm đối với chiều OUT (để không kích hoạt Asymmetric Jump OUT)
    // và nhiễu ngẫu nhiên rời rạc 1 tia đối với chiều IN (để không kích hoạt Jump IN)
    const float noise[5] = {120.0f, 125.0f, 55.0f, 110.0f, 90.0f};
    for (int i = 0; i < 5; ++i)
    {
        FilterResult r = f.process(noise[i]);
        TEST_ASSERT_TRUE_MESSAGE(r.hasOutput, "Luôn giữ output cũ khi chưa có cụm mới");
        TEST_ASSERT_FLOAT_WITHIN(2.0f, old, r.outputCm);
    }
}

// Bước nhảy lớn phải qua HOLD_JUMP (3 lần xác nhận) trước khi ACCEPT_JUMP.
void test_filter_jump_hold_then_accept(void)
{
    DistanceFilter f;
    f.reset();

    for (int i = 0; i < 8; ++i)
    {
        f.process(100.0f);
    }

    bool sawHold = false;
    bool sawAccept = false;
    // Dùng 2 giá trị xen kẽ cách nhau 8.5cm để TRÁNH kích hoạt Fast-Track (yêu cầu <= 8cm)
    // nhưng VẪN tạo thành một cluster (vì cả 2 đều nằm trong dải tolerance quanh giá trị trung bình 34.25)
    for (int i = 0; i < 12; ++i)
    {
        float newTarget = (i % 2 == 0) ? 30.0f : 38.5f;
        FilterResult r = f.process(newTarget);
        if (strcmp(r.status, "HOLD_JUMP") == 0)
        {
            sawHold = true;
            TEST_ASSERT_FLOAT_WITHIN(1.0f, 100.0f, r.outputCm); // chưa đổi
        }
        if (strcmp(r.status, "ACCEPT_JUMP") == 0)
        {
            sawAccept = true;
            TEST_ASSERT_FLOAT_WITHIN(10.0f, 30.0f, r.outputCm); // đã đổi
        }
    }

    TEST_ASSERT_TRUE_MESSAGE(sawHold, "Phải thấy HOLD_JUMP trước khi đổi giá trị");
    TEST_ASSERT_TRUE_MESSAGE(sawAccept, "Phải ACCEPT_JUMP sau khi xác nhận 3 lần");

    float stable = 0.0f;
    TEST_ASSERT_TRUE(f.getStable(stable));
    TEST_ASSERT_FLOAT_WITHIN(10.0f, 30.0f, stable);
}

// Reset xoá sạch trạng thái: getStable chuyển về false.
void test_filter_reset(void)
{
    DistanceFilter f;
    f.reset();

    float out = 0.0f;
    TEST_ASSERT_FALSE(f.getStable(out)); // chưa có gì

    for (int i = 0; i < 8; ++i)
    {
        f.process(100.0f);
    }
    TEST_ASSERT_TRUE(f.getStable(out));

    f.reset();
    TEST_ASSERT_FALSE(f.getStable(out)); // reset -> mất state ổn định
}


// Thử nghiệm khả năng Fast-Track bắt xe chạy nhanh (2 mẫu)
void test_filter_fast_track_crossing(void)
{
    DistanceFilter f;
    f.reset();

    // 1. Giả lập nền thoáng ở 300cm
    for (int i = 0; i < 8; ++i) { f.process(300.0f); }

    // 2. Xe xẹt qua. Mẫu 1 (chiều VÀO): chưa đủ khẳng định, kết quả vẫn giữ nguyên nền cũ
    FilterResult r1 = f.process(50.0f);
    TEST_ASSERT_EQUAL_STRING("OK", r1.status); 
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 300.0f, r1.outputCm);

    // 3. Mẫu 2 giống mẫu 1 -> Xe thật! Fast-Track kích hoạt lập tức
    FilterResult r2 = f.process(52.0f);
    TEST_ASSERT_EQUAL_STRING("FAST_TRACK_CROSSING", r2.status);
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 51.0f, r2.outputCm);
    
    // 4. Xe đã đi qua (chiều RA). "Nhanh vào – chậm ra": phải có FILTER_RELEASE_CONFIRM_SAMPLES
    // mẫu xa liên tiếp mới nhả; trước đó vẫn giữ kết quả gần (không nhảy theo 1 echo xa).
    for (int i = 0; i < FILTER_RELEASE_CONFIRM_SAMPLES - 1; ++i)
    {
        FilterResult rk = f.process(300.0f);
        TEST_ASSERT_FLOAT_WITHIN(5.0f, 51.0f, rk.outputCm);
    }
    FilterResult r3 = f.process(300.0f);
    TEST_ASSERT_EQUAL_STRING("FAST_TRACK_CROSSING", r3.status);
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 300.0f, r3.outputCm);
}

// Echo ảo xa lẻ tẻ (giá trị 190/247/498cm lấy từ log thật) KHÔNG được làm đầu ra nhảy xa
// khi vật vẫn ở gần. Mỗi đợt echo ảo ngắn hơn FILTER_RELEASE_CONFIRM_SAMPLES mẫu.
void test_filter_release_ignores_isolated_far_echoes(void)
{
    DistanceFilter f;
    f.reset();
    for (int i = 0; i < 8; ++i) { f.process(50.0f); }

    const float ghosts[3] = {190.2f, 247.2f, 497.7f};
    const int run = (FILTER_RELEASE_CONFIRM_SAMPLES > 2) ? 2 : 1;
    int g = 0;
    for (int round = 0; round < 8; ++round)
    {
        for (int k = 0; k < run; ++k)
        {
            FilterResult r = f.process(ghosts[g++ % 3]);
            TEST_ASSERT_TRUE(r.hasOutput);
            TEST_ASSERT_FLOAT_WITHIN(10.0f, 50.0f, r.outputCm);
        }
        for (int k = 0; k < 3; ++k)
        {
            FilterResult r = f.process(50.0f);
            TEST_ASSERT_TRUE(r.hasOutput);
            TEST_ASSERT_FLOAT_WITHIN(10.0f, 50.0f, r.outputCm);
        }
    }
}

// N-1 mẫu xa liên tiếp: chưa nhả. Mẫu thứ N: nhả về mẫu GẦN NHẤT trong N mẫu (thận trọng),
// không phải mẫu cuối (có thể là echo ảo rất xa).
void test_filter_release_after_confirmed_far_samples(void)
{
    DistanceFilter f;
    f.reset();
    for (int i = 0; i < 8; ++i) { f.process(50.0f); }

    // Mẫu gần nhất (300) nằm ở vị trí thứ 2 nên đúng với mọi N >= 2.
    const float far[9] = {320.0f, 300.0f, 340.0f, 310.0f, 330.0f, 350.0f, 360.0f, 370.0f, 380.0f};
    for (int i = 0; i < FILTER_RELEASE_CONFIRM_SAMPLES - 1; ++i)
    {
        FilterResult r = f.process(far[i]);
        TEST_ASSERT_FLOAT_WITHIN(10.0f, 50.0f, r.outputCm);
        TEST_ASSERT_TRUE_MESSAGE(strcmp(r.status, "FAST_TRACK_CROSSING") != 0, "Chưa đủ N mẫu xa thì chưa được nhả");
    }
    FilterResult rn = f.process(far[FILTER_RELEASE_CONFIRM_SAMPLES - 1]);
    TEST_ASSERT_EQUAL_STRING("FAST_TRACK_CROSSING", rn.status);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 300.0f, rn.outputCm);
}

// Chuỗi giả-ngẫu-nhiên CỐ ĐỊNH (LCG) để test lặp lại được trên mọi máy.
static uint32_t lcgNext(uint32_t &s)
{
    s = s * 1664525u + 1013904223u;
    return s >> 8;
}

// Mô phỏng log thật: vật cố định ở 22,6cm, mỗi mẫu 20% trả echo ảo xa.
// Bản 21/9 (nhả 1 mẫu) cho ~100/285 đầu ra "xa". Chuỗi cố định này tình cờ có 1 đợt 5 echo ảo
// LIÊN TIẾP (nhả hợp lệ theo thiết kế, ~4 mẫu "xa" trước khi quay lại) nên ngưỡng là <= 10 (~3%).
void test_filter_near_target_with_ghost_echoes_stays_near(void)
{
    const float ghosts[3] = {190.2f, 247.2f, 497.7f};
    DistanceFilter f;
    f.reset();
    uint32_t seed = 12345u;
    int outputs = 0;
    int farOutputs = 0;
    for (int i = 0; i < 300; ++i)
    {
        float raw = 22.6f + (float)(lcgNext(seed) % 7) * 0.1f - 0.3f;
        if ((lcgNext(seed) % 100) < 20)
        {
            raw = ghosts[lcgNext(seed) % 3];
        }
        FilterResult r = f.process(raw);
        if (i >= 15 && r.hasOutput)
        {
            ++outputs;
            if (r.outputCm > 100.0f) { ++farOutputs; }
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(outputs > 250, "Phải có output gần như mọi mẫu sau warmup");
    TEST_ASSERT_TRUE_MESSAGE(farOutputs <= 10, "Echo ảo xa lẻ tẻ không được làm đầu ra nhảy xa");
}

// ─── Test cases bổ sung ───────────────────────────────────────────────────────

void test_filter_two_instances_independent(void)
{
    DistanceFilter fa, fb;
    fa.reset();
    fb.reset();

    // fa: feed 100cm; fb: feed 300cm
    for (int i = 0; i < 10; ++i)
    {
        fa.process(100.0f);
        fb.process(300.0f);
    }

    float a = 0.0f, b = 0.0f;
    TEST_ASSERT_TRUE(fa.getStable(a));
    TEST_ASSERT_TRUE(fb.getStable(b));
    TEST_ASSERT_FLOAT_WITHIN(5.0f, 100.0f, a);
    TEST_ASSERT_FLOAT_WITHIN(5.0f, 300.0f, b);

    // State của fa không ảnh hưởng fb và ngược lại
    TEST_ASSERT_FALSE_MESSAGE(fabsf(a - b) < 10.0f, "Hai instance phải độc lập");
}

// Fill đúng FILTER_HISTORY_SIZE mẫu — không crash, không buffer overflow.
void test_filter_history_full(void)
{
    DistanceFilter f;
    f.reset();

    // Nạp nhiều hơn FILTER_HISTORY_SIZE (9) mẫu — vẫn phải ổn định
    for (int i = 0; i < FILTER_HISTORY_SIZE * 3; ++i)
    {
        FilterResult r = f.process(150.0f);
        (void)r; // không crash là pass
    }

    float stable = 0.0f;
    TEST_ASSERT_TRUE(f.getStable(stable));
    TEST_ASSERT_FLOAT_WITHIN(10.0f, 150.0f, stable);
}

// Input đúng MAX_DISTANCE_CM (500cm) không làm filter crash hay trả về NaN.
void test_filter_max_range_input(void)
{
    DistanceFilter f;
    f.reset();

    for (int i = 0; i < 10; ++i)
    {
        FilterResult r = f.process(MAX_DISTANCE_CM);
        // outputCm không được là NaN
        if (r.hasOutput)
        {
            TEST_ASSERT_FALSE_MESSAGE(r.outputCm != r.outputCm, "outputCm must not be NaN");
            TEST_ASSERT_FLOAT_WITHIN(20.0f, MAX_DISTANCE_CM, r.outputCm);
        }
    }
}

// Ngưỡng ranh giới DANGER (30cm) — bộ lọc phải ổn định đúng vùng nguy hiểm.
void test_filter_stable_at_danger_boundary(void)
{
    DistanceFilter f;
    f.reset();

    const float target = (float)SENSOR_DANGER_CM; // 30cm
    for (int i = 0; i < 10; ++i)
    {
        f.process(target);
    }

    float stable = 0.0f;
    TEST_ASSERT_TRUE(f.getStable(stable));
    TEST_ASSERT_FLOAT_WITHIN(5.0f, target, stable);
}

// Cluster tolerance tăng theo khoảng cách: 2 cụm cách nhau 10cm gần 50cm
// dễ merge hơn ở 50cm (tolerance ~12cm) so với 500cm (tolerance ~48cm).
// Test đảm bảo filter không reject cluster hợp lệ do tolerance quá chặt.
void test_filter_cluster_dynamic_tolerance(void)
{
    // Ở khoảng cách 50cm, tolerance = max(8, 50*0.08) = max(8, 4) = 8cm
    // Feed các mẫu trong dải ±7cm quanh 50cm → phải tạo được cluster
    DistanceFilter f;
    f.reset();

    bool gotOutput = false;
    const float values[] = {50.0f, 57.0f, 44.0f, 53.0f, 48.0f,
                             55.0f, 46.0f, 52.0f, 49.0f};
    for (int i = 0; i < (int)(sizeof(values) / sizeof(values[0])); ++i)
    {
        FilterResult r = f.process(values[i]);
        if (r.hasOutput)
        {
            gotOutput = true;
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(gotOutput, "Cluster trong dải tolerance phải tạo được output");
}

// setup()/loop() nằm ở test_runner.cpp (main duy nhất cho C++ host).