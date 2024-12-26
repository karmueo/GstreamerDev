#include <gst/gst.h>
#include <gtest/gtest.h>

static gboolean eos_received = FALSE;
static gboolean error_received = FALSE;
static gboolean query_received = FALSE;

class GstAppTest : public ::testing::Test
{
protected:
    GstElement *pipeline, *filesrc, *qtdemux, *h264parse, *decoder, *convert1, *filter, *sink;
    GMainLoop *loop;
    GstBus *bus;
    guint watch_id;

    void SetUp() override
    {
        gst_init(nullptr, nullptr);
        pipeline = gst_pipeline_new("test-pipeline");
        filesrc = gst_element_factory_make("filesrc", "my_filesource");
        qtdemux = gst_element_factory_make("qtdemux", "my_demuxer");
        h264parse = gst_element_factory_make("h264parse", "my_parser");
        decoder = gst_element_factory_make("avdec_h264", "my_decoder");
        convert1 = gst_element_factory_make("videoconvert", "my_videoconvert");
        filter = gst_element_factory_make("my_filter", "my_filter");
        sink = gst_element_factory_make("autovideosink", "videosink");
        loop = g_main_loop_new(NULL, FALSE);
    }

    void TearDown() override
    {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(pipeline));
        g_main_loop_unref(loop);
    }
};

// 回调函数定义
static void on_pad_added(GstElement *element, GstPad *pad, gpointer data)
{
    GstPad *sinkpad;
    GstElement *h264parse = (GstElement *)data;
    // 获取 h264parse 元素的静态 sink pad
    sinkpad = gst_element_get_static_pad(h264parse, "sink");
    // 尝试将新添加的 pad 链接到 h264parse 的 sink pad
    if (gst_pad_link(pad, sinkpad) != GST_PAD_LINK_OK)
    {
        g_printerr("Failed to link qtdemux to h264parse.\n");
    }
    gst_object_unref(sinkpad);
}

// 总线消息回调函数定义
static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
    GMainLoop *loop = (GMainLoop *)data;

    switch (GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_EOS:
        eos_received = TRUE;
        g_main_loop_quit(loop);
        break;
    case GST_MESSAGE_ERROR:
    {
        error_received = TRUE;
        g_main_loop_quit(loop);
        break;
    }
    default:
        break;
    }
    return TRUE;
}

/**
 * @brief 查询函数，用于处理对 filter 元素的 srcpad 查询请求
 *
 * @param pad 查询的 pad
 * @param query 查询参数
 * @return gboolean 查询结果
 */
static gboolean gst_my_filter_src_query(gpointer data)
{
    GstElement *pipeline = (GstElement *)data;
    GstCaps *caps;
    // 创建 caps
    caps = gst_caps_new_simple("video/x-raw",
                               "format", G_TYPE_STRING, "I420",
                               "width", G_TYPE_INT, 640,
                               "height", G_TYPE_INT, 480,
                               "framerate", GST_TYPE_FRACTION, 30, 1,
                               NULL);

    // 创建 accept-caps 查询
    GstQuery *query = gst_query_new_accept_caps(caps);
    // 打印查询类型
    g_print("Query type: %s\n", GST_QUERY_TYPE_NAME(query));

    // 处理查询请求，这里以打印查询信息为例
    if (GST_QUERY_TYPE(query) == GST_QUERY_ACCEPT_CAPS)
    {
        g_print("Query: ACCEPT_CAPS\n");
        gst_query_parse_accept_caps(query, &caps);
        g_print("Caps: %s\n", gst_caps_to_string(caps));
        return TRUE;
    }
    return FALSE;
}

/* 定时查询当前播放位置 */
static gboolean query_position(gpointer data)
{
    GstElement *pipeline = (GstElement *)data;
    GstQuery *query;
    gint64 position = 0;

    /* 创建查询对象 */
    query = gst_query_new_position(GST_FORMAT_TIME);

    /* 向管道发出查询请求 */
    if (gst_element_query(pipeline, query))
    {
        gst_query_parse_position(query, NULL, &position);
        g_print("Current position: %" GST_TIME_FORMAT "\n", GST_TIME_ARGS(position));
    }
    else
    {
        g_print("Failed to query current position\n");
    }

    gst_query_unref(query); // 释放查询对象

    return TRUE; // 返回 TRUE 继续定时器
}

// Test setting properties on elements
TEST_F(GstAppTest, SetProperties)
{
    g_object_set(G_OBJECT(filesrc), "location", "../../1.mp4", NULL);
    gchar *location;
    g_object_get(G_OBJECT(filesrc), "location", &location, NULL);
    ASSERT_STREQ(location, "../../1.mp4");
    g_free(location);
}

// Test bus message handling
TEST_F(GstAppTest, BusMessageHandling)
{
    /* 监视管道总线上的消息（注意，这只有在GLib主循环运行时才有效） */
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    watch_id = gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus); // 释放总线对象

    g_object_set(G_OBJECT(filesrc), "location", "/workspaces/GstreamerDev/1.mp4", NULL);

    gst_bin_add_many(GST_BIN(pipeline),
                     filesrc,
                     qtdemux,
                     h264parse,
                     decoder,
                     convert1,
                     filter,
                     sink,
                     NULL); // 将所有元素添加到管道中
    ASSERT_TRUE(gst_element_link(filesrc, qtdemux));

    g_signal_connect(qtdemux, "pad-added", G_CALLBACK(on_pad_added), h264parse);

    ASSERT_TRUE(gst_element_link_many(h264parse,
                                      decoder,
                                      convert1,
                                      filter,
                                      sink,
                                      NULL));

    ASSERT_TRUE(gst_my_filter_src_query(pipeline));
    // gst_my_filter_src_query(pipeline);

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);

    ASSERT_NE(ret, GST_STATE_CHANGE_FAILURE);
    /* 启动定时器，每1秒查询播放位置 */
    g_timeout_add_seconds(1, query_position, pipeline);

    g_main_loop_run(loop);

    g_source_remove(watch_id); // 移除总线监视器

    EXPECT_TRUE(eos_received);    // 检查是否有EOS或错误，确保测试继续运行
    EXPECT_FALSE(error_received); // 检查是否有错误，确保测试继续运行
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}