
const char* ssid = "";
const char* ssidPass = "";
const char* mqtt_server = "";
const uint16_t mqtt_port = 1883;
const char* mqtt_user = "";
const char* mqtt_pwd = "";
const char* mqtt_id = "";

///node/
char pubChan[50] = "";
///+/73616e64626f78/3544cf31fc/#
char subChan[50] = "";
///cnfg/73616e64626f78/3544cf31fc/activate
char cnfgChan[50] = "";
///prsc/73616e64626f78/3544cf31fc
char prscChan[50] = "";
///prsc/73616e64626f78/3544cf31fc/ack
char prscAckChan[50] = "";
///msre/73616e64626f78
char msreChan[50] = "";
///msre/73616e6[50]4626f78/3544cf31fc/rt
char msreRtChan[50] = "";
///msre/73616e64626f78/3544cf31fc/rt/ack
char msreRtAckChan[50] = "";

char prscPayload[256] = "";

char actvPayload[256] = "";
