#include "alloca.h"
#include "stdio.h"
#include <string>
#include "string.h"
#include "floatLog.h"
#include "interface.h"
#include "main.h"
#include "sys/stat.h" // mkdir
#include "sys/time.h" // gettimeofday(()
#include "stringStore.h"
#include "math.h" // pow()
#include "sys/statvfs.h" // statvfs()
#include "sys/resource.h" // getrusage
#include <sys/types.h> // getpid()
#include <unistd.h> // getpid() // sysconf
#include <stdint.h>
#include "unistd.h" // sysconf()
#include <unistd.h> // exec(), JCE, 3-9-2018
#include <iostream>
#include <cstdio>
#include <memory>
#include <curl/curl.h>
#include "interface_ct003.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <cerrno>

using namespace std;

#define DBG(...)
//#define DBG(...) { printf(__VA_ARGS__); printf("\n"); }

static constexpr uint16_t   CT003_PORT      = 12345;
static constexpr size_t     CT003_BUFLEN    = 1024;
static constexpr int        CT003_TIMEOUT_MS= 1000;   // CT antwoordt ~direct
static constexpr int        CT003_RETRIES   = 2;
static const uint8_t        SOH             = 0x01;
static const uint8_t        STX             = 0x02;
static const uint8_t        ETX             = 0x03;
static const std::string    ct_dev          = "HME-3";
static const std::string    ct_mac          = "009c17c28ba8";
static const std::string    self_dev        = "jDaLog";
static const std::string    self_mac        = "020000000001";
static const std::string    self_phase      = "D";

interface_ct003::interface_ct003(const string d, const string n, float i, const char *a):interface(d, n, i), ip(a) 
{
	Pa =    	 		new in(getDescriptor() + "_pa", getName() + " P1", "W", 0);
	Pb = 	        	new in(getDescriptor() + "_pb", getName() + " P2", "W", 0);
	Pc = 	        	new in(getDescriptor() + "_pc", getName() + " P3", "W", 0);
	Pd =       			new in(getDescriptor() + "_pd", getName() + " Ptotaal", "W", 0);
	rssi =     			new in(getDescriptor() + "_rssi", getName() + " rssi", "dBm", 0);
	info_idx = 			new in(getDescriptor() + "_idx", getName() + " info_idx", "", 0);
	low_in = 			new in(getDescriptor() + "_li", getName() + " low_in", "", 0);
	norm_in = 			new in(getDescriptor() + "_ni", getName() + " norm_in", "", 0);
	low_out = 			new in(getDescriptor() + "_lo", getName() + " low_out", "", 0);
	norm_out = 			new in(getDescriptor() + "_no", getName() + " norm_out", "", 0);
	dt =     			new in(getDescriptor() + "_dt", getName() + " fetch time", "", 3);
}

interface_ct003::~interface_ct003()
{
    delete dt;
    delete norm_out;
    delete low_out;
    delete norm_in;
    delete low_in;
    delete info_idx;
    delete rssi;
	delete Pd;
	delete Pc;
	delete Pb;
	delete Pa;
}

// ------------------------------------------------------------ frame building

// Frame: SOH STX <len ascii> |veld|...| ETX <chk 2 ascii-hex>
// len = totale framelengte (telt zichzelf mee, dus iteratief bepalen);
// chk = XOR van SOH t/m ETX als 2 ascii-hex (hoofdletters).
static std::string ct003_checksum(const std::string &frame_so_far)
{
    uint8_t chk = 0;
    for (unsigned char c : frame_so_far) chk ^= c;
    char hex[3];
    snprintf(hex, sizeof hex, "%02x", chk);
    return hex;
}

static std::string ct003_build_frame(const std::vector<std::string> &fields)
{
    std::string body = "|";
    for (size_t i = 0; i < fields.size(); ++i) {
        if (i) body += "|";
        body += fields[i];
    }
    body += "|";

    // len: iteratief — het lenveld is ascii, dus meer cijfers = langer frame.
    size_t len = 0;
    std::string lenstr;
    for (;;) {
        lenstr = std::to_string(len);
        size_t total = 2 + lenstr.size() + body.size() + 1 + 2;
        if (total == len) break;
        if (total > len) { ++len; continue; }   // sprong over het goede punt
        len = total;                            // nog te klein, direct springen
    }
    std::string f;
    f += (char)SOH; f += (char)STX; f += lenstr; f += body; f += (char)ETX;
    f += ct003_checksum(f);
    return f;
}

static bool ct003_check_frame(const char *buf, size_t len, std::string &body)
{
    if (len < 10) return false;
    if ((uint8_t)buf[0] != SOH || (uint8_t)buf[1] != STX) return false;
    // len-veld: ascii-decimaal direct na STX. len = totale framelengte
    // T/M ETX — de 2 checksum-tekens staan erbuiten (zie live-test:
    // len=129, frame 131 bytes). Dit is het scheermes: alles wat de CT
    // als lengte opgeeft, is het frame. Daarbuiten ligt alleen rommel
    // (of de checksum) — nooit mee parsen.
    size_t p = 2;
    size_t declared = 0;
    while (p < len && buf[p] >= '0' && buf[p] <= '9') {
        declared = declared * 10 + (size_t)(buf[p] - '0');
        ++p;
        if (declared > CT003_BUFLEN) return false;    // onzin-lengte
    }
    if (p == 2) return false;                         // geen len-digits
    if (declared < p + 1 || declared > len)       // frame + chk moet er zijn
        return false;
    if ((uint8_t)buf[declared - 3] != ETX)             // ETX precies op len-3
        return false;

    // checksum over SOH t/m ETX (== buf[0 .. declared-2)), 2 ascii-hex erbuiten
    string want(buf + declared-2, 2);
    string got = ct003_checksum(string(buf, declared-2));
    if (want != got)
        return false;
    body.assign(buf, declared - 3);                   // afgeknipt, ETX eraf
    return true;
}

void field_to_in(const string &s, in *i)
{
    float f;
    int rv = sscanf(s.c_str(), "%f", &f);
    if (rv == 1)
        i->setVal(f);
}

void interface_ct003::ct003_parse_body(const string &body)
{
    // "SOH STX <len> |veld|veld|..." — skip 2 headerbytes + len-digits
    size_t p = 2;
    while (p < body.size() && body[p] >= '0' && body[p] <= '9') ++p;

    std::vector<std::string> f;
    size_t s = p;
    for (size_t i = p; i <= body.size(); ++i) 
    {
        if (i == body.size() || body[i] == '|') 
        {
            if (i > s) 
                f.push_back(body.substr(s, i - s));
            if (i == body.size()) 
                break;
            s = i + 1;
        }
    }
    if ( f.size() >= 28 )
    {
        field_to_in( f[4], Pa );
        field_to_in( f[5], Pb );
        field_to_in( f[6], Pc );
        field_to_in( f[7], Pd );
        field_to_in( f[12], rssi );
        field_to_in( f[13], info_idx );
        field_to_in( f[24], low_in );
        field_to_in( f[25], norm_in );
        field_to_in( f[26], low_out );
        field_to_in( f[27], norm_out );
    }
}


void interface_ct003::getIns()
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd <= 0)
    {
        DBG("Opening socket failed.");
        return;
    }
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(CT003_PORT);
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1)
    {
        DBG("inet_pton() failed");
        close(fd);
        return;
    }
    struct timeval tv{ CT003_TIMEOUT_MS / 1000, (CT003_TIMEOUT_MS % 1000) * 1000};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
    string req = ct003_build_frame(
    {
        self_dev,
        self_mac,
        ct_dev,
        ct_mac,
        self_phase,
        "0",
        "0"
    }); 
    DBG(req.c_str());
	double t1 = now(); 
    if (sendto(fd, req.data(), req.size(), 0, (sockaddr *) &addr, sizeof addr) < 0)
    {
        DBG("Send failed.");
        close(fd);
        return;
    }
    char buf[CT003_BUFLEN] = {0};
    ssize_t n = recv(fd, buf, CT003_BUFLEN, 0); // Socket timeout set earlier.
	double t2 = now(); 
    dt->setVal(t2-t1);
    if (n < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            DBG("Timeout");
        }
        DBG("Failed receiving.");
        close(fd);
        return;
    }
    DBG("Received:");
    DBG(buf);
    string body;
    if (!ct003_check_frame(buf, (size_t)n, body))
    {
        DBG("Checksum / frame failure.");
        close(fd);
        return;
    }    
    
    ct003_parse_body(body); 


    close(fd);
}

interface_ct003* interface_ct003_from_json(const json_t *json )
{
	#define JSTR(_X_)	json_string_value(json_object_get(json, #_X_))
	#define JNR(_X_)	json_number_value(json_object_get(json, #_X_))
	#define JIN(_X_)	json_is_number(json_object_get(json, #_X_))
		
	const char *id = JSTR(id);
	const char *name = JSTR(name);
	if (not name)
		name = id;
	float scan = JNR(scan);
	const char *ip = JSTR(address);
	
	if (id and name and JIN(scan) and ip)
		return new interface_ct003(id, name, scan, ip);
	printf("could not build interface_ct003(%s, %s, %f, %s)\n", id, name, scan, ip);
	return NULL;	
}







