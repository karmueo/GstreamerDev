#include <gst/gst.h>

/**
 * @brief 处理GStreamer总线消息的回调函数。
 *
 * @param bus 指向GstBus的指针。
 * @param msg 指向GstMessage的指针，表示接收到的消息。
 * @param data 用户数据指针。
 *
 * @return 如果消息被成功处理，返回TRUE；否则返回FALSE。
 */
static gboolean
bus_call(GstBus *bus,
         GstMessage *msg,
         gpointer data)
{
    GMainLoop *loop = data;

    switch (GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_EOS: // End-of-stream 消息，表示流结束
        g_print("End-of-stream\n");
        g_main_loop_quit(loop);
        break;
    case GST_MESSAGE_ERROR: // 错误消息
    {
        gchar *debug = NULL;
        GError *err = NULL;

        // 解析错误消息，获取错误信息和调试信息
        gst_message_parse_error(msg, &err, &debug);

        g_print("Error: %s\n", err->message);
        g_error_free(err);

        if (debug)
        {
            g_print("Debug details: %s\n", debug);
            g_free(debug);
        }

        g_main_loop_quit(loop);
        break;
    }
    default:
        break;
    }

    return TRUE;
}

/**
 * @brief 当一个 pad 被添加到元素时的回调函数。
 *
 * @param element 触发此回调的 GstElement 元素。在这里是 qtdemux 元素。
 * @param pad 新添加的 GstPad。这里表示 qtdemux 元素动态创建的 pad。
 * @param data 用户数据指针，可以在回调中使用。在这里是 h264parse 元素。
 */
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

gint main(gint argc,
          gchar *argv[])

{
    GstStateChangeReturn ret; // 定义状态改变返回值变量
    GstElement *pipeline,
        *filesrc,
        *decoder,
        *qtdemux,
        *h264parse,
        *filter,
        *sink;            // 定义GStreamer元素指针变量
    GstElement *convert1; // 定义GStreamer元素指针变量
    GMainLoop *loop;      // 定义主循环指针变量
    GstBus *bus;          // 定义总线指针变量
    guint watch_id;       // 定义监视ID变量

    /* 初始化 */
    gst_init(&argc, &argv);              // 初始化GStreamer库
    loop = g_main_loop_new(NULL, FALSE); // 创建新的主循环
    if (argc != 2)                       // 检查命令行参数数量
    {
        g_print("Usage: %s <mp3 filename>\n", argv[0]); // 打印使用方法
        return 01;                                      // 返回错误代码01
    }

    /* 创建元素 */
    pipeline = gst_pipeline_new("my_pipeline"); // 创建新的管道元素

    /* 监视管道总线上的消息（注意，这只有在GLib主循环运行时才有效） */
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline)); // 获取管道的总线
    watch_id = gst_bus_add_watch(bus, bus_call, loop);  // 添加总线监视器
    gst_object_unref(bus);                              // 释放总线对象

    // "filesrc"：这是要创建的元素的类型名称。在 GStreamer 中，filesrc 元素用于从文件中读取数据。
    // "my_filesource"：这是创建的元素的名称。这个名称是用户定义的，可以在管道中唯一标识该元素。
    filesrc = gst_element_factory_make("filesrc", "my_filesource"); // 创建文件源元素
    qtdemux = gst_element_factory_make("qtdemux", "my_demuxer");    // 创建 MP4 解析器元素
    h264parse = gst_element_factory_make("h264parse", "my_parser"); // 创建 H.264 解析器元素
    decoder = gst_element_factory_make("avdec_h264", "my_decoder"); // 创建 H.264 解码器元素

    /* 在这里放置一个audioconvert元素，将解码器的输出转换为my_filter可以处理的格式 */
    convert1 = gst_element_factory_make("videoconvert", "my_videoconvert"); // 创建视频转换器元素

    /* 使用"identity"作为不做任何处理的过滤器 */
    filter = gst_element_factory_make("my_filter", "my_filter"); // 创建自定义过滤器元素

    // convert2 = gst_element_factory_make("audioconvert", "audioconvert2");  // 创建第二个音频转换器元素
    // resample = gst_element_factory_make("audioresample", "audioresample"); // 创建音频重采样元素
    sink = gst_element_factory_make("autovideosink", "videosink"); // 创建视频接收器元素

    if (!sink || !decoder) // 检查接收器和解码器是否创建成功
    {
        g_print("Decoder or output could not be found - check your install\n"); // 打印错误信息
        return -1;                                                              // 返回错误代码-1
    }
    else if (!convert1) // 检查转换器是否创建成功
    {
        g_print("Could not create videoconvert element, check your installation\n"); // 打印错误信息
        return -1;                                                                   // 返回错误代码-1
    }
    else if (!filter) // 检查自定义过滤器是否创建成功
    {
        g_print("Your self-written filter could not be found. Make sure it is installed correctly in $(libdir)/gstreamer-1.0/ or ~/.gstreamer-1.0/plugins/ and that gst-inspect-1.0 lists it. If it doesn't, check with 'GST_DEBUG=*:2 gst-inspect-1.0' for the reason why it is not being loaded."); // 打印错误信息
        return -1;                                                                                                                                                                                                                                                                                    // 返回错误代码-1
    }

    g_object_set(G_OBJECT(filesrc), "location", argv[1], NULL); // 设置文件源的文件路径

    gst_bin_add_many(GST_BIN(pipeline),
                     filesrc,
                     qtdemux,
                     h264parse,
                     decoder,
                     convert1,
                     filter,
                     sink,
                     NULL); // 将所有元素添加到管道中

    // 在 GStreamer 管道中，filesrc 和 qtdemux 的链接方式与其他元素有所不同。
    // qtdemux 元素会动态地创建 pad，因此不能像其他元素那样直接链接。需要使用回调函数来处理。
    // 链接文件源和解析器
    if (!gst_element_link(filesrc, qtdemux))
    {
        g_printerr("Filesrc and qtdemux could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    // 连接解析器的 pad 到解析器的 pad
    g_signal_connect(qtdemux, "pad-added", G_CALLBACK(on_pad_added), h264parse);

    /* 将所有元素链接在一起 */
    if (!gst_element_link_many(h264parse,
                               decoder,
                               convert1,
                               filter,
                               sink,
                               NULL)) // 链接所有元素
    {
        g_printerr("Failed to link one or more elements!\n"); // 打印错误信息
        gst_object_unref(pipeline);
        return -1; // 返回错误代码-1
    }

    /* 运行 */
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING); // 将管道状态设置为播放
    if (ret == GST_STATE_CHANGE_FAILURE)                      // 检查状态改变是否失败
    {
        GstMessage *msg; // 定义消息指针变量

        g_printerr("Failed to start up pipeline!\n"); // 打印错误信息

        /* 检查总线上是否有错误消息 */
        msg = gst_bus_poll(bus, GST_MESSAGE_ERROR, 0); // 轮询总线上的错误消息
        if (msg)                                       // 如果有错误消息
        {
            GError *err = NULL; // 定义错误指针变量

            gst_message_parse_error(msg, &err, NULL); // 解析错误消息
            g_print("ERROR: %s\n", err->message);     // 打印错误信息
            g_error_free(err);                        // 释放错误对象
            gst_message_unref(msg);                   // 释放消息对象
        }
        return -1; // 返回错误代码-1
    }

    g_main_loop_run(loop); // 运行主循环

    /* 清理 */
    gst_element_set_state(pipeline, GST_STATE_NULL); // 将管道状态设置为NULL
    gst_object_unref(pipeline);                      // 释放管道对象
    g_source_remove(watch_id);                       // 移除总线监视器
    g_main_loop_unref(loop);                         // 释放主循环对象

    return 0; // 返回成功代码0
}