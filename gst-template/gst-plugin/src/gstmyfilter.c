#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gst/gst.h>

#include "gstmyfilter.h"

GST_DEBUG_CATEGORY_STATIC(gst_my_filter_debug); // 定义静态调试类别
#define GST_CAT_DEFAULT gst_my_filter_debug     // 设置默认调试类别

/* 过滤器信号和参数 */
enum
{
    /* 填写信号 */
    LAST_SIGNAL
};

enum
{
    PROP_0,
    PROP_SILENT // 静默属性
};

/* 输入和输出的能力描述
 *
 * 在这里描述实际的格式
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE("sink",
                                                                   GST_PAD_SINK,
                                                                   GST_PAD_ALWAYS,
                                                                   GST_STATIC_CAPS("ANY")); // 定义静态Pad模板，类型为sink，始终存在，支持任意格式

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE("src",
                                                                  GST_PAD_SRC,
                                                                  GST_PAD_ALWAYS,
                                                                  GST_STATIC_CAPS("ANY")); // 定义静态Pad模板，类型为src，始终存在，支持任意格式

static gboolean gst_my_filter_src_query(GstPad *pad,
                                        GstObject *parent,
                                        GstQuery *query);

#define gst_my_filter_parent_class parent_class
G_DEFINE_TYPE(GstMyFilter, gst_my_filter, GST_TYPE_ELEMENT); // 定义GstMyFilter类型
// 宏 GST_ELEMENT_REGISTER_DEFINE 与 GST_ELEMENT_REGISTER_DECLARE 相结合，允许通过调用 GST_ELEMENT_REGISTER (my_filter) 从插件内或任何其他插件/应用程序中注册元素。
GST_ELEMENT_REGISTER_DEFINE(my_filter, "my_filter", GST_RANK_NONE, GST_TYPE_MYFILTER); // 注册元素

static void gst_my_filter_set_property(GObject *object,
                                       guint prop_id, const GValue *value, GParamSpec *pspec); // 设置属性函数声明
static void gst_my_filter_get_property(GObject *object,
                                       guint prop_id, GValue *value, GParamSpec *pspec); // 获取属性函数声明

static gboolean gst_my_filter_sink_event(GstPad *pad,
                                         GstObject *parent, GstEvent *event); // 处理sink事件函数声明
static GstFlowReturn gst_my_filter_chain(GstPad *pad,
                                         GstObject *parent, GstBuffer *buf); // 处理数据链函数声明

/* GObject 虚方法实现 */

/* _class_init() 函数，仅用于初始化类一次（指定类具有哪些信号、参数和虚函数并设置全局状态）
 */
static void
gst_my_filter_class_init(GstMyFilterClass *klass)
{
    GObjectClass *gobject_class;       // 定义GObjectClass指针
    GstElementClass *gstelement_class; // 定义GstElementClass指针

    gobject_class = (GObjectClass *)klass;       // 将klass转换为GObjectClass并赋值给gobject_class
    gstelement_class = (GstElementClass *)klass; // 将klass转换为GstElementClass并赋值给gstelement_class

    gobject_class->set_property = gst_my_filter_set_property; // 设置属性函数
    gobject_class->get_property = gst_my_filter_get_property; // 获取属性函数

    // 设置一个bool类型的属性，名称为"silent"，默认值为FALSE
    g_object_class_install_property(
        gobject_class,
        PROP_SILENT,
        g_param_spec_boolean(
            "silent",                  // 属性名
            "Silent",                  // nickname
            "Produce verbose output?", // 描述
            FALSE,                     // 默认值为FALSE
            G_PARAM_READWRITE)         // 读写属性
    );

    // 设置元素详细信息，元素名称为"MyFilter"，分类为"FIXME:Generic"，描述为"FIXME:Generic Template Element"，作者为"ytkj <<user@hostname.org>>"
    gst_element_class_set_details_simple(gstelement_class,
                                         "MyFilter",
                                         "FIXME:Generic",
                                         "FIXME:Generic Template Element",
                                         "ytkj <<user@hostname.org>>");

    // 添加src pad模板
    gst_element_class_add_pad_template(gstelement_class,
                                       gst_static_pad_template_get(&src_factory));
    // 添加sink pad模板
    gst_element_class_add_pad_template(gstelement_class,
                                       gst_static_pad_template_get(&sink_factory));
}

/* 初始化新元素
 * 实例化pads并将其添加到元素中
 * 设置pad回调函数
 * 初始化实例结构
 */
static void
gst_my_filter_init(GstMyFilter *filter)
{
    filter->sinkpad = gst_pad_new_from_static_template(&sink_factory, "sink"); // 从静态模板创建sink pad
    gst_pad_set_event_function(filter->sinkpad,
                               GST_DEBUG_FUNCPTR(gst_my_filter_sink_event)); // 设置sink pad事件函数
    gst_pad_set_chain_function(filter->sinkpad,
                               GST_DEBUG_FUNCPTR(gst_my_filter_chain)); // 设置sink pad链函数

    // 在将pad添加到元素之前，在pad上配置事件函数
    gst_pad_set_query_function(filter->srcpad,
                               gst_my_filter_src_query);

    // filter->sinkpad 将会自动代理其连接的srcpad 的 caps。
    // 这意味着 sinkpad 将继承并传播其上游元素的 caps，从而确保数据格式的一致性和兼容性
    GST_PAD_SET_PROXY_CAPS(filter->sinkpad);                   // 设置代理能力
    gst_element_add_pad(GST_ELEMENT(filter), filter->sinkpad); // 将sink pad添加到元素中

    filter->srcpad = gst_pad_new_from_static_template(&src_factory, "src"); // 从静态模板创建src pad
    // filter->srcpad 将会自动代理其连接的 sinkpad 的 caps。
    // 这意味着 srcpad 将继承并传播其下游元素的 caps，从而确保数据格式的一致性和兼容性
    GST_PAD_SET_PROXY_CAPS(filter->srcpad);                   // 设置代理能力
    gst_element_add_pad(GST_ELEMENT(filter), filter->srcpad); // 将src pad添加到元素中

    filter->silent = FALSE; // 初始化静默属性为FALSE
}

static void
gst_my_filter_set_property(GObject *object, guint prop_id,
                           const GValue *value, GParamSpec *pspec)
{
    GstMyFilter *filter = GST_MYFILTER(object);

    switch (prop_id)
    {
    case PROP_SILENT:
        filter->silent = g_value_get_boolean(value); // 设置静默属性
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec); // 无效属性ID警告
        break;
    }
}

static void
gst_my_filter_get_property(GObject *object, guint prop_id,
                           GValue *value, GParamSpec *pspec)
{
    GstMyFilter *filter = GST_MYFILTER(object);

    switch (prop_id)
    {
    case PROP_SILENT:
        g_value_set_boolean(value, filter->silent); // 获取静默属性
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec); // 无效属性ID警告
        break;
    }
}

/* GstElement 虚方法实现 */

/* 处理sink事件的函数 */
static gboolean
gst_my_filter_sink_event(GstPad *pad, GstObject *parent,
                         GstEvent *event)
{
    GstMyFilter *filter;
    gboolean ret;

    filter = GST_MYFILTER(parent);

    GST_LOG_OBJECT(filter, "receive %s event: %" GST_PTR_FORMAT,
                   GST_EVENT_TYPE_NAME(event), event); // 记录收到的事件

    switch (GST_EVENT_TYPE(event))
    {
    case GST_EVENT_CAPS:
    {
        GstCaps *caps;

        gst_event_parse_caps(event, &caps); // 解析事件中的caps
        /* 处理caps：例如检查 caps 的格式、调整管道中的元素等。 */

        /* 转发事件 */
        ret = gst_pad_event_default(pad, parent, event); // 默认事件处理
        break;
    }
    default:
        ret = gst_pad_event_default(pad, parent, event); // 默认事件处理
        break;
    }
    return ret;
}

/* 链函数
 * 这个函数进行实际处理
 */
static GstFlowReturn
gst_my_filter_chain(GstPad *pad, GstObject *parent, GstBuffer *buf)
{
    GstMyFilter *filter;

    filter = GST_MYFILTER(parent);

    if (!filter->silent)
        g_print("Have data of size %" G_GSIZE_FORMAT " bytes!\n",
                gst_buffer_get_size(buf));

    /* 直接推送输入缓冲区，不做任何处理 */
    return gst_pad_push(filter->srcpad, buf);
}

/**
 * @brief 处理源查询的回调函数。
 *
 * 这个函数用于处理从src pad 发出的查询请求。
 *
 * @param pad 指向 GstPad 的指针，表示源 pad。
 * @param parent 指向 GstObject 的指针，表示父对象。
 * @param query 指向 GstQuery 的指针，表示查询请求。
 *
 * @return gboolean 如果查询处理成功，返回 TRUE；否则返回 FALSE。
 */
static gboolean
gst_my_filter_src_query(GstPad *pad,
                        GstObject *parent,
                        GstQuery *query)
{
    gboolean ret;

    switch (GST_QUERY_TYPE(query))
    {
    case GST_QUERY_POSITION:
        /* we should report the current position */
        gint64 position;

        if (gst_element_query_position(GST_ELEMENT(parent), GST_FORMAT_TIME, &position))
        {
            gst_query_set_position(query, GST_FORMAT_TIME, position);
            ret = TRUE;
        }
        else
        {
            ret = FALSE;
        }

        break;
    case GST_QUERY_DURATION:
        /* we should report the duration here */
        // 获取时长
        gint64 duration;

        if (gst_element_query_duration(GST_ELEMENT(parent), GST_FORMAT_TIME, &duration))
        {
            gst_query_set_duration(query, GST_FORMAT_TIME, duration);
            ret = TRUE;
        }
        else
        {
            ret = FALSE;
        }
        break;
    case GST_QUERY_CAPS:
        /* we should report the supported caps here */
        // 获取能力
        GstCaps *caps;

        caps = gst_pad_get_current_caps(pad);
        gst_query_set_caps_result(query, caps);
        break;
    default:
        /* just call the default handler */
        ret = gst_pad_query_default(pad, parent, query);
        break;
    }
    return ret;
}

/* 初始化插件的入口点
 * 初始化插件本身
 * 注册元素工厂和其他特性
 */
static gboolean
myfilter_init(GstPlugin *myfilter)
{
    /* 调试类别，用于过滤日志消息
     *
     * 将字符串 'Template myfilter' 替换为你的描述
     */
    GST_DEBUG_CATEGORY_INIT(gst_my_filter_debug,
                            "myfilter",
                            0,
                            "Template myfilter");

    return GST_ELEMENT_REGISTER(my_filter, myfilter); // 注册元素
}

/* PACKAGE: 通常由meson根据meson.build中的某些_INIT宏设置，然后写入并定义在config.h中，但我们可以
 * 在这里自己设置，以防有人不使用meson来编译此代码。GST_PLUGIN_DEFINE需要定义PACKAGE。
 */
#ifndef PACKAGE
#define PACKAGE "myfirstmyfilter"
#endif

/* gstreamer查找此结构以注册过滤器
 *
 * 将字符串 'Template myfilter' 替换为你的过滤器描述
 */
GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  myfilter,
                  "my_filter",
                  myfilter_init,
                  PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
