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

    // Các mẫu rời rạc trải khắp dải đo: làm cụm cũ vỡ, không cụm mới đủ 5 phiếu
    const float noise[5] = {200.0f, 300.0f, 55.0f, 150.0f, 90.0f};
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

    // 2. Xe xẹt qua. Mẫu 1: chưa đủ khẳng định, kết quả vẫn giữ nguyên nền cũ
    FilterResult r1 = f.process(50.0f);
    TEST_ASSERT_EQUAL_STRING("OK", r1.status); // Vẫn tìm thấy cụm 300cm trong lịch sử
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 300.0f, r1.outputCm);

    // 3. Mẫu 2 giống mẫu 1 -> Xe thật! Fast-Track kích hoạt lập tức
    FilterResult r2 = f.process(52.0f);
    TEST_ASSERT_EQUAL_STRING("FAST_TRACK_CROSSING", r2.status);
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 51.0f, r2.outputCm);
    
    // 4. Xe đã đi qua, trả về nền cũ. Mẫu 3 chưa đủ khẳng định (vẫn giữ 51cm chờ xác nhận).
    FilterResult r3 = f.process(300.0f);
    TEST_ASSERT_EQUAL_STRING("WARMUP", r3.status); // Lịch sử đã bị reset, chờ thu thập đủ 5 mẫu mới
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 51.0f, r3.outputCm);

    // 5. Mẫu 4 giống mẫu 3 -> Khoảng trống thật. Nền kéo về 300cm lập tức.
    FilterResult r4 = f.process(300.0f);
    TEST_ASSERT_EQUAL_STRING("FAST_TRACK_CROSSING", r4.status);
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 300.0f, r4.outputCm);
}

// ─── Test cases bổ sung ───────────────────────────────────────────────────────

// Hai instance DistanceFilter hoạt động hoàn toàn độc lập (mô phỏng 2 cảm biến).
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