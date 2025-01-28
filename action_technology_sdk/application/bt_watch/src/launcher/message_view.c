/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <app_ui.h>
#include <widgets/simple_img.h>
#include <widgets/text_canvas.h>

#define MSG_CONTENT_PAGE_NUM		(3)

enum {
	APP_SMS = 0,
	APP_WECHAT,
	APP_OTHER,

	NUM_APP_TYPES,
};

typedef struct {
	/* message application type */
	uint8_t type;
	/* message content */
	const char *name;
	const char *text;
} message_desc_t;

typedef struct {
	lvgl_res_group_t res_list;
	lvgl_res_group_t res_list_preview;
	lvgl_res_string_t res_list_str[2];
	lv_point_t pt_list_icon;

	lvgl_res_group_t res_content;
	lvgl_res_string_t res_content_str[2];
	lv_point_t pt_content_icon;

	lvgl_res_scene_t scene;
} message_view_tmp_res_t;

typedef struct {
	lv_obj_t * list;
	lv_area_t list_area;

	lv_style_t sty_list;
	lv_style_t sty_list_app_icon;
	//lv_style_t sty_list_app_name;
	lv_style_t sty_list_btn;
	lv_style_t sty_list_name;
	lv_style_t sty_list_text;
	//lv_style_t sty_time;

	lv_obj_t * box;
	lv_area_t box_area;

	lv_style_t sty_box;
	lv_style_t sty_box_app_icon;
	lv_style_t sty_box_name;
	lv_style_t sty_box_text;

	lv_font_t font;
	lv_font_t font_small;

	/* lvgl resource */
	lv_image_dsc_t img_icons[NUM_APP_TYPES];

	bool gesture_locked;
} message_view_data_t;

static int _show_message_list(view_data_t *view_data);
static int _show_message_content(view_data_t *view_data, const message_desc_t *msg);

static void _message_view_unload_resource(message_view_data_t *data);

#ifdef CONFIG_BITMAP_FONT_SUPPORT_EMOJI
static uint8_t emojitxt[64] = {0xf0,0x9f,0x98,0x88,0xE4,0xB8,0xA4,0xf0,0x9f,0x98,0x81,0xE5,0xA4,0xA9,0xf0,0x9f,0x98,0x89,0xE5,0x8F,0x88,0xf0,0x9f,0x98,0x83,0xE6,0x9D,0xA5,0xE4,0xBA,0x86,0xE4,0xB8,0xAA,0xE6,0x80,0xA5,0xE5,0x88,0xB9,0xf0,0x9f,0x98,0x86,0xE8,0xBD,0xA6,0xf0,0x9f,0x98,0x82,0xf0,0x9f,0x98,0x80,0xf0,0x9f,0x98,0x84,0x20,0};
#endif

static const message_desc_t messages[10] = {
#ifdef CONFIG_BITMAP_FONT_SUPPORT_EMOJI
	[0] = {
		.type = APP_WECHAT,
		.name = "比特币",
		.text = emojitxt,
	},
#else
	[0] = {
		.type = APP_WECHAT,
		.name = "比特币",
		.text = "比特币刚刚反弹到5万美元以上，这两天又来了个急刹车，连续暴跌，9月22日凌晨一度跌破了4万美元大关，回到了8月6日以来的最低点。受此影响，过去24小时中有17.5万人爆仓，损失的资金高达70亿元，如果算上20日当天大约百亿元的爆仓，这两天来就有170亿元爆仓，部分投资者损失惨重。从9月20日开始，比特币就开始进入下行车道，当时是因为美国股市暴跌，纳指跌了700多点，引发恐慌。昨晚美国股市立马反弹了，但比特币等数字货币并没有涨起来，因为美国最热的币圈交易平台Coinbase的加密货币贷款项目被监管部门反对。		",
	},
#endif

	[1] = {
		.type = APP_SMS,
		.name = "美联储Taper",
		.text = "当经济遭遇危机时，美联储会采取量化宽松的手段来刺激经济。而当经济逐渐从危机之中恢复，就业率和通胀指标接近或达到美联储设立的目标线时，美联储就会开始减缓放水的速度，逐渐退出量化宽松。Taper一词来源于古希腊语，本义是指用于在烛灯中慢慢燃烧殆尽的细而长的烛芯，在现代被形象地引申为形容美联储试图在尽量不惊扰经济复苏的前提下，逐步缩减购债规模、退出量化宽松的行动。从以往的经验来看，美联储Taper通常遵循以下几个步骤：释放Taper信号——宣布Taper——开始Taper操作——完成Taper。目前，美联储正。",
	},
	[2] = {
		.type = APP_OTHER,
		.name = "中秋档票房",
		.text = "21世纪经济报道，北京报道电影中秋档成绩惨淡。9月22日凌晨，21世纪经济报道记者依据国家电影专资办所公布数据初步计算，2021年电影“中秋档”（9月19日—9月21日）票房约4.955171亿元，处于历史低谷。灯塔数据显示，从2018年到2020年，电影“中秋档”票房依次达到5.31亿元、8.04亿元、7.45亿元。这意味着，今年中秋档，为四年来最差。据国家电影专资办数据，朱一龙主演《峰爆》，在中秋档拿下2.14亿票房，为票房冠军，上映总票房2.72亿元；徐帆主演《关于我妈的一切》次之，中秋档创新纪录。",
	},
	[3] = {
		.type = APP_WECHAT,
		.name = "北京环球影城",
		.text = "尽管开业时间一拖再拖，北京环球度假区（包含“北京环球影城”、城市大道、酒店；以下统称为北京环球影城）9月20日开园门票还是被一抢而空。系统崩溃、付款失败、抢到票后被强制退票……预订系统开放首日，北京环球度假区官方就不得不出面道歉。实际上，正式开业前的一个月，已有将近25万人提前游玩。《每日经济新闻》记者了解到，无论是内测还是试运营，环球影城的票价均在2000元以上，最热时到过5000元一张。明星、KOL相继打卡，种种营销声浪让北京环球影城的开业热度力压上海迪士尼开业时的盛况。每经记者两度实探北京环球影城。",
	},
	[4] = {
		.type = APP_SMS,
		.name = "中国疫苗",
		.text = "在实施为期18个月的禁令后，美国11月重新开放，允许“完全接种新冠疫苗”的外国人入境。据美国《纽约时报》21日报道，在实施为期18个月的禁令后，美国将于11月重新开放，允许“完全接种新冠疫苗”（fully vaccinated）的外国人入境。美国疾控中心（CDC）发言人称，所有接种了获得世界卫生组织紧急使用认证的疫苗的人，均将视为“完全接种”。美国此前并不认可世卫组织批准紧急使用的所有疫苗，例如阿斯利康疫苗。目前我国科兴疫苗、国药疫苗已被列入世卫组织紧急使用清单。美国政府此举，则是“变相认可”了中国疫苗。",
	},
	[5] = {
		.type = APP_OTHER,
		.name = "黑芝麻智能",
		.text = "9月22日，自动驾驶计算芯片厂商“黑芝麻智能”宣布今年已经完成数亿美元的战略轮及C轮两轮融资。战略轮由小米长江产业基金，富赛汽车等参与投资；C轮融资由小米长江产业基金领投，闻泰战投、武岳峰资本、天际资本、元禾璞华、联想创投、临芯资本、中国汽车芯片产业创新战略联盟等跟投。战略轮及C轮融资投后，“黑芝麻智能”估值近20亿美元，目前C+轮融资也在推进中。目前，黑芝麻智能与中国一汽、博世、上汽、东风悦享等在L2/3级ADAS和自动驾驶感知系统解决方案上开展了商业合作。黑芝麻智能创始人兼单记章表示十分感谢汽车产业。",
	},

	[6] = {
		.type = APP_OTHER,
		.name = "埃及象形文字",
		.text = "1799年，一支法国部队探险小组在埃及发现了罗塞塔石碑，这为破译埃及象形文字奠定了基础，这块神秘石碑上刻有托勒密五世的法令，以三种文字铭刻：埃及象形文字、通俗文字(公元前7世纪至公元5世纪埃及人使用的文字)和古希腊文。该法令铭刻于公元前196年，宣布祭司为托勒密五世加冕，并且减免税收。据悉，发现罗塞塔石碑时，埃及象形文字和通俗文字都还没有被破译，但人们知道其中铭刻的古希腊文，事实上，相同的法令以3种语言保存下来，意味着现代学者可以阅读石碑中的古希腊文部分，并将其与象形文字和通俗文字进行比较，从而确定相同。",
	},
	[7] = {
		.type = APP_WECHAT,
		.name = "古代遗骸",
		.text = "7200年前，一位女性被埋在印度尼西亚境内，她属于一支以前不为人知的人类谱系，现已灭绝消失。同时，该女性的基因组还表明，她是当今澳大利亚土著和美拉尼西亚人(即新几内亚和西太平洋岛屿上的土著居民)的远亲，与澳大利亚澳大利亚土著和新几内亚人一样，该女子的DNA中有很大一部分来自丹尼索瓦人，这与其他东南亚远古狩猎人类形成鲜明差别，老挝和马来西亚等地区的人类没有丹尼索瓦人的遗传基因。德国图宾根大学人类进化和古环境中心研究员波斯特森肯堡(Posth Senckenberg)称，这些基因发现表晨，在印尼和周围的岛屿。",
	},
	[8] = {
		.type = APP_SMS,
		.name = "猛犸",
		.text = "研究表明，大约1.7万年前生活在现今阿拉斯加地区的猛犸行程很远、范围很广，如果它沿着一条直线路径行走，一生可以环绕地球近两圈。在28年里，它行走了接近8.05万公里，为了追溯成年猛犸的足迹，研究人员做了一件之前从未做过的事情——他们沿着猛犸长牙切开，分析长牙中积累的化学成分。然后他们将这些数据和阿拉斯加各地的化学物质特征进行了比较，这些化学特征是从冰河时期小型哺乳动物牙齿中识别出来，通常将象牙不同部位的化学元素比例与小型哺乳动物牙齿的相似比例进行匹配，科学家就能绘制出一幅区域地图，呈现猛犸每年生活的区域。",
	},
	[9] = {
		.type = APP_OTHER,
		.name = "iPhone 13",
		.text = "依稀记得去年iPhone 12系列发售的时候，就有不少用户说，“王守义告诉我们十三才香，大家还是等等iPhone 13”。现如今，全新iPhone 13系列已如期而至，无论是9月15日凌晨发布当天的多条热搜，还是9月17日晚间国内开启预售的空前火爆，无不向我们透露出：“老先生诚不欺，13系列果然很香”。跟去年iPhone 12系列一样，今年iPhone 13系列依然是三个尺寸版本的四个机型，而其中最受关注的自然还是超大杯iPhone 13 Pro Max（售价8999元起），大屏体验很难再回去适应小屏机。",
	},
/*
	[10] = {
		.type = APP_WECHAT,
		.name = "二手房交易",
		.text = "随着“放管服”改革的深化和粤港澳大湾区建设的推进，珠海不动产登记服务不断优化，企业群众的办事体验感实现了质的飞跃。近日，珠海市不动产登记中心与广州市不动产登记中心密切合作，实现了首单二手房过户的“跨城通办”！一步也没有离开居住地广州，两人异地交易了一套位于珠海的二手房，买房的曾先生与卖房的吴先生直呼“想不到！”“太满意了！”40岁的曾先生与来自澳门的吴先生是多年的老朋友，先后定居广州。两人近期聊天时，吴先生提到了想出售自己在珠海的一套房产，曾先生正好想买，两人达成交易意向。但因疫情原因，两人都不方便到珠。",
	},
	[11] = {
		.type = APP_SMS,
		.name = "全运会",
		.text = "好戏连台“人气指数”不断刷新，牛！第十四届全运会火热进行中，本次全运会中，几乎所有国内高水平运动员都参与了此次大赛竞技，他们为我们带来许多精彩的较量，也让全运会“人气指数”不断刷新。网友感叹：“这些全运会的热搜，每一条都好喜欢！东京奥运会女子铅球冠军巩立姣20日以19米88夺得第十四届全运会该项目金牌，实现全运会四连冠。在21日晚结束的全运会男子200米自由泳决赛的比赛中，东京奥运会冠军汪顺获得该项目的冠军，至此，汪顺在全运会的金牌总数达到13枚，超越孙杨和广东游泳名宿罗兆应，登顶全运会历史的“多金王”。",
	},
	[12] = {
		.type = APP_OTHER,
		.name = "丰收节",
		.text = "第四个“中国农民丰收节”到来,习近平向全国广大农民和工作在“三农”战线上的同志们致以节日祝贺和诚挚慰问,强调加快农业农村现代化，让广大农民生活芝麻开花节节高。今年以来，我们克服新冠肺炎疫情、洪涝自然灾害等困难，粮食和农业生产喜获丰收，农村和谐稳定，农民幸福安康，对开新局、应变局、稳大局发挥了重要作用。民族要复兴，乡村必振兴。进入实现第二个百年奋斗目标新征程，“三农”工作重心已历史性转向全面推进乡村振兴。各级党委和政府要贯彻党中央关于“三农”工作大政方针和决策部署，坚持农业农村优先发展，加快农业农村现代化。",
	},
	[13] = {
		.type = APP_OTHER,
		.name = "香港选委会",
		.text = "新华社香港9月21日电2021年香港特别行政区选举委员会界别分组一般选举日前成功举行，新一届选委会顺利产生。很多香港基层市民表示，期待落实新选举制度给香港社会带来新气象，希望新一届选委会能发挥为香港繁荣稳定把关的作用。“香港正在由乱变治，再变兴旺。希望在完善选举制度后，新当选的选委能够为香港把关，选出好的行政长官和立法会议员。市民欧阳先生表示，不少香港基层市民仍面临住房问题，一些人还住在劏房笼屋，希望选委们积极服务地区街坊，协助特区政府推进有关民生政策。最大的希望就是特区政府能够帮助解决年轻人的住房问题。",
	},
	[14] = {
		.type = APP_WECHAT,
		.name = "拉闸限电",
		.text = "9月22日，中秋节后第一个交易日，A股银行板块大跌，市场对恒大流动性危机的担忧蔓延开来，对此，相关银行机构披露与恒大业务情况。22日晚间，刷屏有一大批上市公司披露了限电限产影响，可以直接说是直接放假到国庆节了。西大门22日晚间公告称，由于电力供应紧张，浙江省近日对辖区内重点用能企业实行用电降负荷，在确保安全的前提下对重点用能企业实施停产，预计将停产至9月30日。浙江西大门新材料股份有限公司（以下简称“公司”）目前被迫临时停产，预计影响遮阳面料产量约11.54万平方米/日。具体影响效益情况暂时无法准确预计。",
	},
	[15] = {
		.type = APP_SMS,
		.name = "环保产业",
		.text = "推动100个左右地级及以上城市开展“无废城市”建设，到2025年，城市固体废物产生强度稳步下降，综合利用水平和比例大幅提升，区域处置设施缺口基本补齐，减污降碳协同增效作用明显，基本实现固体废物管理信息“一张网”,“无废”理念得到广泛认同，固体废物治理体系和治理能力得到明显提升。征求意见稿的内容主要包含加快工业发展方式绿色转型，降低工业固体废物贮存填埋量；发展农业循环经济，促进主要农业废弃物综合利用；践行绿色生活方式，提高生活垃圾减量化、资源化水平；加强全过程管理，推进建筑垃圾综合利用；强化监管和处置能力。",
	},
	[16] = {
		.type = APP_OTHER,
		.name = "芯片大牛股",
		.text = "作为哈勃科技投资的第一家企业，思瑞浦是一家专注于模拟集成电路产品研发和销售的集成电路设计企业。公司产品以信号链模拟芯片为主，营收占比在9成以上，近年来逐渐向电源管理模拟芯片拓展，其应用范围涵盖信息通讯、工业控制、监控安全、医疗健康、仪器仪表和家用电器等领域。2019年5月，思瑞浦原股东与哈勃投资签署《投资协议》，后者认购金额合计7200万元，单价32.13元/股。华为的加持，吸引了更多投资机构加入。2019年12月，思瑞浦完成第三次股份转让、嘉兴君齐、合肥润广、惠友创嘉、惠友创享、元禾璞华等进入股东之列。",
	},
	[17] = {
		.type = APP_WECHAT,
		.name = "医企IPO",
		.text = "9月22日，吉凯基因科创板首发上会被否，这是继海和药物之后，本月第二家被否决的生物医药企业。同一日，上交所披露终止审核海和药物首发申请的公告。在披露的终止审核及被否公告中，监管层对吉凯基因和海和药物的“科创含量”存疑，产品研发能力和是否具备较高的技术壁垒是科创板上市委多次问询的话题。近年来，由于允许未盈利企业上市等利好，强调科技创新属性的科创板吸引了越来越多药企的目光。今年4月，证监会、上交所修改《科创板属性评价指引》，进一步强调科创板定位。随着监管要求越来越严格，“伪创新”的生物医药企业也将被洗牌出局。",
	},
	[18] = {
		.type = APP_SMS,
		.name = "煤化工",
		.text = "煤化工起源于18世纪工业革命后，广泛应用于冶金、农业领域。所谓的煤化工是指以煤炭为原料，经过物理和化学反应转化为气体、液体和固体燃料以及化学品的过程。煤化工一般分为传统煤化工和现代煤化工，其中传统煤化工产品包括碳黑、PVC、合成氨等；而现代煤化工则以替代石油路线为基本目标，使用煤炭生产化学品，包括各类烯烃、芳烃、甲醇、乙醇、乙二醇、汽柴油、醋酸等。煤化工产业的崛起主要是基于我国“多煤少油缺气”的资源格局。我国是世界上第一大煤炭生产国及消费国，探明能源储量中，煤炭占94%，石油占5.4%，天然气占0.6%。",
	},
	[19] = {
		.type = APP_OTHER,
		.name = "酒店行业",
		.text = "受新冠疫情影响，酒旅行业遭受重创，入住率出现断崖式下降，加速了行业优胜劣汰的过程。伴随着我国疫情管控得当、旅游板块逐步复苏，2021年上半年，我国酒店行业持续复苏趋势，A股酒店企业业绩回暖明显。根据Wind行业中心数据显示，在A股，目前共有7家酒店上市公司，其中5家酒店上市公司的营业收入已经出现不同程度上涨。作为全球第七大酒店集团的华住集团，未在A股上市，但其业绩也表现出了增长韧性。酒店行业增量市场萎缩，存量市场的争夺愈演愈烈。以锦江酒店、华住集团、首旅酒店为主的三大酒店龙头扩张提速，尽享集中度提升红利。",
	},
*/
};

static const uint32_t res_list_str_ids[] = {
	STR_CONTACT, STR_BRIEF,
};

static const uint32_t res_content_str_ids[] = {
	STR_TITLE, STR_CONTENT,
};

static int8_t message_preload_inited = 0;

static void _message_view_set_gesture_locked(message_view_data_t *data, bool locked)
{
	if (data->gesture_locked == locked)
		return;

	if (locked) {
		ui_gesture_lock_scroll();
	} else {
		ui_gesture_unlock_scroll();
	}

	data->gesture_locked = locked;
}

static int _message_view_load_resource(message_view_data_t *data, message_view_tmp_res_t *res, bool first_layout)
{
	lvgl_res_picregion_t picregion;
	lvgl_res_group_t res_subgrp;
	int ret;

	ret = lvgl_res_load_scene(SCENE_MSG_VIEW, &res->scene, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (ret < 0) return ret;

	ret = lvgl_res_load_group_from_scene(&res->scene, RES_GRP_LIST, &res->res_list);
	if (ret < 0) goto out_exit;

	ret = lvgl_res_load_picregion_from_group(&res->res_list, RES_ICON, &picregion);
	if (ret < 0) {
		lvgl_res_unload_group(&res->res_list);
		goto out_exit;
	}

	ret = lvgl_res_load_pictures_from_picregion(&picregion, 0, NUM_APP_TYPES - 1, data->img_icons);
	if (ret < 0) {
		lvgl_res_unload_picregion(&picregion);
		lvgl_res_unload_group(&res->res_list);
		goto out_exit;
	}

	res->pt_list_icon.x = picregion.x;
	res->pt_list_icon.y = picregion.y;
	lvgl_res_unload_picregion(&picregion);

	ret = lvgl_res_load_group_from_group(&res->res_list, RES_PREVIEW, &res->res_list_preview);
	if (ret < 0) {
		lvgl_res_unload_group(&res->res_list);
		goto out_exit;
	}

	lvgl_res_unload_group(&res->res_list);

	ret = lvgl_res_load_strings_from_group(&res->res_list_preview, res_list_str_ids, res->res_list_str, ARRAY_SIZE(res->res_list_str));
	if (ret < 0) {
		lvgl_res_unload_group(&res->res_list_preview);
		goto out_exit;
	}

	lvgl_res_unload_strings(res->res_list_str, ARRAY_SIZE(res->res_list_str));
	lvgl_res_unload_group(&res->res_list_preview);

	if (first_layout) {
		ret = lvgl_res_load_group_from_scene(&res->scene, RES_GRP_CONTENT, &res->res_content);
		if (ret < 0) goto out_exit;

		ret = lvgl_res_load_strings_from_group(&res->res_content, res_content_str_ids, res->res_content_str, ARRAY_SIZE(res_content_str_ids));
		if (ret < 0) {
			lvgl_res_unload_group(&res->res_content);
			goto out_exit;
		}

		lvgl_res_unload_strings(res->res_content_str, ARRAY_SIZE(res->res_content_str));

		ret = lvgl_res_load_group_from_group(&res->res_content, RES_ICON, &res_subgrp);
		if (ret < 0) {
			lvgl_res_unload_group(&res->res_content);
			goto out_exit;
		}

		res->pt_content_icon.x = res_subgrp.x;
		res->pt_content_icon.y = res_subgrp.y;
		lvgl_res_unload_group(&res_subgrp);
		lvgl_res_unload_group(&res->res_content);

		ret = LVGL_FONT_OPEN_DEFAULT(&data->font, DEF_FONT_SIZE_NORMAL);
		if (ret < 0) goto out_exit;

		ret = LVGL_FONT_OPEN_DEFAULT(&data->font_small, DEF_FONT_SIZE_SMALL);
		if (ret < 0) goto out_exit;


#ifdef CONFIG_BITMAP_FONT_SUPPORT_EMOJI
		LVGL_FONT_SET_EMOJI(&data->font, DEF_FONT28_EMOJI);
		LVGL_FONT_SET_EMOJI(&data->font_small, DEF_FONT28_EMOJI);
		LVGL_FONT_SET_DEFAULT_CODE(&data->font, 0x53E3, 0x1ffff);
		LVGL_FONT_SET_DEFAULT_CODE(&data->font_small, 0x53E3, 0x1ffff);
#endif
	}

out_exit:
	lvgl_res_unload_scene(&res->scene);
	return ret;
}

static void _message_view_unload_pic_resource(message_view_data_t *data)
{
    lvgl_res_unload_pictures(data->img_icons, ARRAY_SIZE(data->img_icons));
}

static void _message_view_unload_resource(message_view_data_t *data)
{
	LVGL_FONT_CLOSE(&data->font);
	LVGL_FONT_CLOSE(&data->font_small);
	lvgl_res_unload_pictures(data->img_icons, ARRAY_SIZE(data->img_icons));
}

static int _message_view_preload(view_data_t *view_data, bool update)
{
	if (message_preload_inited == 0) {
		lvgl_res_preload_scene_compact_default_init(SCENE_MSG_VIEW, NULL, 0,
			NULL, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
		message_preload_inited = 1;
	}

	return lvgl_res_preload_scene_compact_default(SCENE_MSG_VIEW, MSG_VIEW, update, 0);
}

static void _message_list_btn_event_handler(lv_event_t * e)
{
	view_data_t *view_data = lv_event_get_user_data(e);
	message_view_data_t *data = view_data->user_data;
	lv_obj_t *btn = lv_event_get_target(e);
	int index = (int)lv_obj_get_user_data(btn);

	lv_obj_add_flag(data->list, LV_OBJ_FLAG_HIDDEN);
	_show_message_content(view_data, &messages[index]);
}

#ifndef CONFIG_VIEW_SCROLL_TRACKING_FINGER
static void _message_list_event_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	message_view_data_t *data = lv_event_get_user_data(e);

	if (code == LV_EVENT_SCROLL) {
		if (lv_obj_get_scroll_top(data->list) <= 0) {
			_message_view_set_gesture_locked(data, false);
		} else {
			_message_view_set_gesture_locked(data, true);
		}
	}
}
#endif /* CONFIG_VIEW_SCROLL_TRACKING_FINGER */

static int _show_message_list(view_data_t *view_data)
{
	message_view_data_t *data = view_data->user_data;
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);

	data->list = lv_obj_create(scr);
	lv_obj_add_style(data->list, &data->sty_list, LV_PART_MAIN);
	lv_obj_set_flex_flow(data->list, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_scroll_dir(data->list, LV_DIR_VER);

	for (int i = 0; i < ARRAY_SIZE(messages); i++) {
		lv_obj_t *btn = lv_button_create(data->list);

		lv_obj_add_style(btn, &data->sty_list_btn, LV_PART_MAIN);
		lv_obj_set_user_data(btn, (void *)i);
		lv_obj_add_event_cb(btn, _message_list_btn_event_handler, LV_EVENT_SHORT_CLICKED, view_data);

		lv_obj_t *obj_appicon = simple_img_create(btn);
		lv_obj_add_style(obj_appicon, &data->sty_list_app_icon, LV_PART_MAIN);
		simple_img_set_src(obj_appicon, &data->img_icons[messages[i].type]);

		lv_obj_t *obj_name = text_canvas_create(btn);
		lv_obj_add_flag(obj_name, LV_OBJ_FLAG_EVENT_BUBBLE);
		lv_obj_remove_flag(obj_name, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_style(obj_name, &data->sty_list_name, LV_PART_MAIN);
		text_canvas_set_text_static(obj_name, messages[i].name);

		lv_obj_t *obj_text = text_canvas_create(btn);
		lv_obj_add_flag(obj_text, LV_OBJ_FLAG_EVENT_BUBBLE);
		lv_obj_remove_flag(obj_text, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_style(obj_text, &data->sty_list_text, LV_PART_MAIN);
#ifdef CONFIG_BITMAP_FONT_SUPPORT_EMOJI
		text_canvas_set_emoji_enable(obj_text, true);
#endif
		/* use default TEXT_CANVAS_LONG_WRAP for performance */
		//text_canvas_set_long_mode(obj_text, TEXT_CANVAS_LONG_DOT);
		text_canvas_set_text_static(obj_text, messages[i].text);
	}

#ifndef CONFIG_VIEW_SCROLL_TRACKING_FINGER
	lv_obj_add_event_cb(data->list, _message_list_event_handler, LV_EVENT_ALL, data);
#endif

	return 0;
}

static void _message_content_event_handler(lv_event_t * e)
{
	message_view_data_t *data = lv_event_get_user_data(e);
	lv_indev_t *indev = lv_event_get_param(e);
	lv_dir_t dir = lv_indev_get_gesture_dir(indev);

	if (data->box && dir == LV_DIR_RIGHT) {
		lv_indev_wait_release(indev);
		lv_obj_delete(data->box);
		data->box = NULL;
		lv_obj_remove_flag(data->list, LV_OBJ_FLAG_HIDDEN);

		/* restore system gesture */
#ifndef CONFIG_VIEW_SCROLL_TRACKING_FINGER
		if (lv_obj_get_scroll_top(data->list) <= 0) {
			_message_view_set_gesture_locked(data, false);
		}
#else
		_message_view_set_gesture_locked(data, false);
#endif
	}
}

static int _show_message_content(view_data_t *view_data, const message_desc_t *msg)
{
	message_view_data_t *data = view_data->user_data;
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);

	data->box = lv_obj_create(scr);
	lv_obj_add_style(data->box, &data->sty_box, LV_PART_MAIN);

	lv_obj_t *obj_appicon = simple_img_create(data->box);
	lv_obj_add_style(obj_appicon, &data->sty_box_app_icon, LV_PART_MAIN);
	simple_img_set_src(obj_appicon, &data->img_icons[msg->type]);

	lv_obj_t *obj_name = lv_label_create(data->box);
	lv_obj_add_style(obj_name, &data->sty_box_name, LV_PART_MAIN);
	lv_label_set_long_mode(obj_name, LV_LABEL_LONG_CLIP /* LV_LABEL_LONG_SCROLL_CIRCULAR */);
	lv_label_set_text(obj_name, msg->name);

	lv_obj_t *obj_text_box = lv_obj_create(data->box);
	lv_obj_add_flag(obj_text_box, LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
	lv_obj_add_style(obj_text_box, &data->sty_box_text, LV_PART_MAIN);
	lv_obj_set_scroll_dir(obj_text_box, LV_DIR_VER);

	lv_obj_t *obj_text = text_canvas_create(obj_text_box);
	lv_obj_set_width(obj_text, LV_PCT(100));
	lv_obj_set_style_max_height(obj_text, lv_obj_get_style_height(obj_text_box, 0) * MSG_CONTENT_PAGE_NUM, 0);
	lv_obj_add_flag(obj_text, LV_OBJ_FLAG_EVENT_BUBBLE);
#ifdef CONFIG_BITMAP_FONT_SUPPORT_EMOJI
	text_canvas_set_emoji_enable(obj_text, true);
#endif
	text_canvas_set_text_static(obj_text, msg->text);

	/* refresh the visible content ASAP */
	lv_refr_now(view_data->display);

	/* set system gesture */
	_message_view_set_gesture_locked(data, true);

	return 0;
}

static int _message_view_layout_update(view_data_t *view_data, bool first_layout)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	message_view_tmp_res_t tmp_res;
	message_view_data_t *data = view_data->user_data;

	if (first_layout) {
		data = app_mem_malloc(sizeof(*data));
		if (!data) {
			return -ENOMEM;
		}
		memset(data, 0, sizeof(*data));
		view_data->user_data = data;
	}

	if (_message_view_load_resource(data, &tmp_res, first_layout)) {
		_message_view_unload_resource(data);
		app_mem_free(data);
		return -ENOENT;
	}

	if (first_layout) {
		lv_style_init(&data->sty_list);
		lv_style_set_x(&data->sty_list, tmp_res.res_list.x);
		lv_style_set_y(&data->sty_list, tmp_res.res_list.y);
		lv_style_set_width(&data->sty_list, tmp_res.res_list.width);
		lv_style_set_height(&data->sty_list, tmp_res.res_list.height);

		lv_style_init(&data->sty_list_btn);
		lv_style_set_x(&data->sty_list_btn, tmp_res.res_list_preview.x);
		lv_style_set_y(&data->sty_list_btn, tmp_res.res_list_preview.y);
		lv_style_set_width(&data->sty_list_btn, LV_PCT(100));
		lv_style_set_height(&data->sty_list_btn, tmp_res.res_list_preview.height);

		lv_style_init(&data->sty_list_app_icon);
		lv_style_set_x(&data->sty_list_app_icon, tmp_res.pt_list_icon.x);
		lv_style_set_y(&data->sty_list_app_icon, tmp_res.pt_list_icon.y);

		lv_style_init(&data->sty_list_name);
		lv_style_set_x(&data->sty_list_name, tmp_res.res_list_preview.x + tmp_res.res_list_str[0].x);
		lv_style_set_y(&data->sty_list_name, tmp_res.res_list_preview.y + tmp_res.res_list_str[0].y);
		lv_style_set_width(&data->sty_list_name, tmp_res.res_list_str[0].width);
		lv_style_set_height(&data->sty_list_name, tmp_res.res_list_str[0].height);
		lv_style_set_text_align(&data->sty_list_name, LV_TEXT_ALIGN_LEFT);
		lv_style_set_text_font(&data->sty_list_name, &data->font);
		lv_style_set_text_color(&data->sty_list_name, tmp_res.res_list_str[0].color);

		lv_style_init(&data->sty_list_text);
		lv_style_set_x(&data->sty_list_text, tmp_res.res_list_preview.x + tmp_res.res_list_str[1].x);
		lv_style_set_y(&data->sty_list_text, tmp_res.res_list_preview.y + tmp_res.res_list_str[1].y);
		lv_style_set_width(&data->sty_list_text, tmp_res.res_list_str[1].width);
		lv_style_set_height(&data->sty_list_text, tmp_res.res_list_str[1].height);
		lv_style_set_text_align(&data->sty_list_text, LV_TEXT_ALIGN_LEFT);
		lv_style_set_text_font(&data->sty_list_text, &data->font);
		lv_style_set_text_color(&data->sty_list_text, tmp_res.res_list_str[1].color);

		lv_style_init(&data->sty_box);
		lv_style_set_x(&data->sty_box, tmp_res.res_content.x);
		lv_style_set_y(&data->sty_box, tmp_res.res_content.y);
		lv_style_set_width(&data->sty_box, tmp_res.res_content.width);
		lv_style_set_height(&data->sty_box, tmp_res.res_content.height);

		lv_style_init(&data->sty_box_app_icon);
		lv_style_set_x(&data->sty_box_app_icon, tmp_res.pt_content_icon.x);
		lv_style_set_y(&data->sty_box_app_icon, tmp_res.pt_content_icon.y);

		lv_style_init(&data->sty_box_name);
		lv_style_set_x(&data->sty_box_name, tmp_res.res_content_str[0].x);
		lv_style_set_y(&data->sty_box_name, tmp_res.res_content_str[0].y);
		lv_style_set_width(&data->sty_box_name, tmp_res.res_content_str[0].width);
		lv_style_set_height(&data->sty_box_name, tmp_res.res_content_str[0].height);
		lv_style_set_pad_hor(&data->sty_box_name, 6);
		lv_style_set_radius(&data->sty_box_name, 6);
		lv_style_set_bg_color(&data->sty_box_name, tmp_res.res_content_str[0].bgcolor);
		lv_style_set_bg_opa(&data->sty_box_name, LV_OPA_COVER);
		lv_style_set_text_align(&data->sty_box_name, LV_TEXT_ALIGN_LEFT);
		lv_style_set_text_font(&data->sty_box_name, &data->font);
		lv_style_set_text_color(&data->sty_box_name, tmp_res.res_content_str[0].color);

		lv_style_init(&data->sty_box_text);
		lv_style_set_x(&data->sty_box_text, tmp_res.res_content_str[1].x);
		lv_style_set_y(&data->sty_box_text, tmp_res.res_content_str[1].y);
		lv_style_set_width(&data->sty_box_text, tmp_res.res_content_str[1].width);
		lv_style_set_height(&data->sty_box_text, tmp_res.res_content_str[1].height);
		lv_style_set_text_align(&data->sty_box_text, LV_TEXT_ALIGN_LEFT);
		lv_style_set_text_font(&data->sty_box_text, &data->font_small);
		lv_style_set_text_color(&data->sty_box_text, tmp_res.res_content_str[1].color);

		lv_obj_set_style_bg_color(scr, tmp_res.scene.background, LV_PART_MAIN);
		lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

		/* add gesture event */
		lv_obj_add_event_cb(scr, _message_content_event_handler, LV_EVENT_GESTURE, data);
		lv_obj_remove_flag(scr, LV_OBJ_FLAG_GESTURE_BUBBLE);
	}

	return 0;
}

static int _message_view_layout(view_data_t *view_data)
{
	int ret;

	ret = _message_view_layout_update(view_data, true);
	if (ret < 0)
		return ret;

	_show_message_list(view_data);

	lv_refr_now(view_data->display);

	return 0;
}

static int _message_view_delete(view_data_t *view_data)
{
	message_view_data_t *data = view_data->user_data;

	if (data) {
		lv_obj_delete(data->list);
		if (data->box)
			lv_obj_delete(data->box);

		lv_style_reset(&data->sty_list);
		lv_style_reset(&data->sty_list_app_icon);
		lv_style_reset(&data->sty_list_btn);
		lv_style_reset(&data->sty_list_name);
		lv_style_reset(&data->sty_list_text);
		lv_style_reset(&data->sty_box);
		lv_style_reset(&data->sty_box_app_icon);
		lv_style_reset(&data->sty_box_name);
		lv_style_reset(&data->sty_box_text);

		_message_view_unload_resource(data);
		_message_view_set_gesture_locked(data, false);
		app_mem_free(data);
	} else {
		lvgl_res_preload_cancel_scene(SCENE_MSG_VIEW);
	}

	lvgl_res_unload_scene_compact(SCENE_MSG_VIEW);
	return 0;
}

static int _message_view_updated(view_data_t* view_data)
{
	return _message_view_layout_update(view_data, false);
}

static int _message_view_focus_changed(view_data_t *view_data, bool focused)
{
	message_view_data_t *data = view_data->user_data;

	if (focused) {
		if (!lvgl_res_scene_is_loaded(SCENE_MSG_VIEW))
			_message_view_preload(view_data, true);
	} else {
		if (data)
			_message_view_unload_pic_resource(data);

		lvgl_res_preload_cancel_scene(SCENE_MSG_VIEW);
		lvgl_res_unload_scene_compact(SCENE_MSG_VIEW);
	}

	return 0;
}

int _message_view_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void *msg_data)
{
	assert(view_id == MSG_VIEW);

	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _message_view_preload(view_data, false);
	case MSG_VIEW_LAYOUT:
		return _message_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _message_view_delete(view_data);
	case MSG_VIEW_FOCUS:
		return _message_view_focus_changed(view_data, true);
	case MSG_VIEW_DEFOCUS:
		return _message_view_focus_changed(view_data, false);
	case MSG_VIEW_UPDATE:
		return _message_view_updated(view_data);
	case MSG_VIEW_PAINT:
	default:
		return 0;
	}
}

VIEW_DEFINE2(message, _message_view_handler, NULL,
		NULL, MSG_VIEW, HIGH_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
