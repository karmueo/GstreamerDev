# 查找 Google Test 依赖
gtest = dependency('gtest', required: true)

# 测试源文件
test_sources = files('test_demo.cpp')

# 创建可执行文件
test_demo_exe = executable('test_demo', test_sources, dependencies: [gst_dep, gtest])

# 注册 Meson 测试
test('test_demo', test_demo_exe)