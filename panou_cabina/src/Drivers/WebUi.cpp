#if defined(WEB_UI_IMPLEMENTATION)

#include "WebUi.h"

#include "Config.h"
#include "ConfigurationService.h"
#include "ServiceMenu.h"
#include "ServiceProtocol.h"
#include "SharedPanel.h"
#include "UpdateService.h"
#include "WebUiContent.h"

#include <Arduino.h>
#include <Ethernet_Generic.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace WebUi
{
namespace
{
    // ------------------------------------------------------------------
    // Limits and shared state
    // ------------------------------------------------------------------

    constexpr size_t PATH_SIZE = 96;
    constexpr size_t HEADER_LINE_SIZE = 160;
    constexpr size_t BODY_SIZE = 96;
    constexpr uint32_t REQUEST_TIMEOUT_MS = 3000;
    constexpr uint32_t UPLOAD_TIMEOUT_MS = 30000;
    constexpr uint8_t POLL_BYTE_BUDGET = 96;
    constexpr uint8_t UPLOAD_CHUNK_BUDGET = 8;   // iteratii x 512 B per poll

    struct Request
    {
        EthernetClient client;
        char line[HEADER_LINE_SIZE] = {};
        size_t lineLength = 0;
        char method[8] = {};
        char path[PATH_SIZE] = {};
        char authorization[112] = {};
        char host[72] = {};
        char origin[96] = {};
        char contentType[40] = {};
        char uploadFileName[48] = {};
        char body[BODY_SIZE] = {};
        size_t bodyLength = 0;
        size_t contentLength = 0;
        uint32_t lastByteAt = 0;
        bool headersDone = false;
        bool active = false;
        bool overflow = false;
        bool isUploadStream = false;   // POST /api/update/upload (binar)
    };

    EthernetServer sServer(Config::Web::PORT);
    Request sRequest;
    bool sCapturingResponse = false;
    char sServiceResponse[96] = {};

    void resetRequest()
    {
        sRequest = Request{};
    }

    // ------------------------------------------------------------------
    // Responses
    // ------------------------------------------------------------------

    void sendResponse(int status, const char *reason, const char *type,
                      const char *body, size_t length, bool noStore,
                      const char *extraHeader = nullptr)
    {
        EthernetClient &client = sRequest.client;
        char header[512];
        int headerLength = snprintf(header, sizeof(header),
            "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %u\r\n"
            "Connection: close\r\nX-Content-Type-Options: nosniff\r\n"
            "Content-Security-Policy: default-src 'self'; style-src 'self'; script-src 'self'; connect-src 'self'; object-src 'none'; frame-ancestors 'none'\r\n"
            "Cache-Control: %s\r\n",
            status, reason, type, static_cast<unsigned>(length),
            noStore ? "no-store" : "public, max-age=3600");
        if (headerLength > 0 && extraHeader != nullptr) {
            const int appended = snprintf(header + headerLength, sizeof(header) - static_cast<size_t>(headerLength),
                                          "%s", extraHeader);
            if (appended > 0) headerLength += appended;
        }
        if (headerLength > 0 && static_cast<size_t>(headerLength) + 2 < sizeof(header)) {
            header[headerLength++] = '\r';
            header[headerLength++] = '\n';
            client.write(reinterpret_cast<const uint8_t *>(header), static_cast<size_t>(headerLength));
        }
        // NOTE: Ethernet_Generic's EthernetClient::write() internally chunks
        // and blocks until everything is sent, but always returns 0 (library
        // bug) - do not loop on its return value, just ignore it.
        if (length > 0) client.write(reinterpret_cast<const uint8_t *>(body), length);
        client.stop();
        resetRequest();
    }

    void sendText(int status, const char *reason, const char *body)
    {
        sendResponse(status, reason, "text/plain; charset=utf-8", body, strlen(body), true);
    }

    void sendFlash(int status, const char *reason, const char *type, const char *body)
    {
        sendResponse(status, reason, type, body, strlen(body), true);
    }

    // ------------------------------------------------------------------
    // Authentication (HTTP Basic, fixed username "service")
    // ------------------------------------------------------------------

    enum class AuthResult : uint8_t
    {
        Ok,
        PasswordNotConfigured,
        MissingHeader,
        BadEncoding,
        BadCredentials
    };

    bool decodeBasicAuth(const char *encoded, char *decoded, size_t capacity)
    {
        static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        uint32_t buffer = 0;
        uint8_t bits = 0;
        size_t outputLength = 0;
        for (const char *cursor = encoded; *cursor != '\0' && *cursor != '='; ++cursor) {
            const char *found = strchr(alphabet, *cursor);
            if (found == nullptr) return false;
            buffer = (buffer << 6) | static_cast<uint32_t>(found - alphabet);
            bits = static_cast<uint8_t>(bits + 6);
            if (bits >= 8) {
                bits = static_cast<uint8_t>(bits - 8);
                if (outputLength + 1 >= capacity) return false;
                decoded[outputLength++] = static_cast<char>((buffer >> bits) & 0xFF);
            }
        }
        decoded[outputLength] = '\0';
        return outputLength > 0;
    }

    AuthResult checkAuth()
    {
        // sRequest.authorization holds only the base64 token; the
        // "Authorization: Basic " scheme was matched during header parsing.
        if (Config::Web::AUTH_PASSWORD[0] == '\0') return AuthResult::PasswordNotConfigured;
        if (sRequest.authorization[0] == '\0') return AuthResult::MissingHeader;
        char decoded[80];
        if (!decodeBasicAuth(sRequest.authorization, decoded, sizeof(decoded))) return AuthResult::BadEncoding;
        char expected[64];
        const int length = snprintf(expected, sizeof(expected), "%s:%s",
                                    Config::Web::AUTH_USERNAME, Config::Web::AUTH_PASSWORD);
        if (length <= 0 || static_cast<size_t>(length) >= sizeof(expected)) return AuthResult::BadCredentials;
        return strcmp(decoded, expected) == 0 ? AuthResult::Ok : AuthResult::BadCredentials;
    }

    bool requireAuth()
    {
        const AuthResult result = checkAuth();
        if (result == AuthResult::Ok) return true;
        const char *message;
        switch (result) {
            case AuthResult::PasswordNotConfigured:
                message = "Parola WEB_UI_PASSWORD nu este configurata in firmware";
                break;
            case AuthResult::MissingHeader:
                message = "Lipseste headerul Authorization";
                break;
            case AuthResult::BadEncoding:
                message = "Header Authorization invalid (base64)";
                break;
            default:
                message = "Nume utilizator sau parola incorecte";
                break;
        }
        sendResponse(401, "Unauthorized", "text/plain; charset=utf-8", message, strlen(message), true,
                     "WWW-Authenticate: Basic realm=\"panou-lift\", charset=\"UTF-8\"\r\n");
        return false;
    }

    // ------------------------------------------------------------------
    // Request validation helpers
    // ------------------------------------------------------------------

    bool sameOrigin()
    {
        if (sRequest.host[0] == '\0' || sRequest.origin[0] == '\0') return false;
        char expected[104];
        const int length = snprintf(expected, sizeof(expected), "http://%s", sRequest.host);
        return length > 0 && static_cast<size_t>(length) < sizeof(expected) && strcmp(sRequest.origin, expected) == 0;
    }

    bool plainTextRequest()
    {
        if (strncmp(sRequest.contentType, "text/plain", 10) != 0 ||
            (sRequest.contentType[10] != '\0' && sRequest.contentType[10] != ';')) return false;
        for (size_t i = 0; i < sRequest.bodyLength; i++) {
            const uint8_t value = static_cast<uint8_t>(sRequest.body[i]);
            if (value < 0x20 || value > 0x7E) return false;
        }
        return true;
    }

    bool requireSameOrigin()
    {
        if (sameOrigin()) return true;
        sendText(403, "Forbidden", "Origine invalida sau lipsa");
        return false;
    }

    bool requirePlainTextBody()
    {
        if (plainTextRequest()) return true;
        sendText(415, "Unsupported Media Type", "Este acceptat doar text/plain ASCII");
        return false;
    }

    void terminateBody()
    {
        sRequest.body[sRequest.bodyLength] = '\0';
    }

    bool parseIPv4(const char *text, uint8_t address[4])
    {
        unsigned octets[4];
        char trailing;
        if (sscanf(text, "%u.%u.%u.%u%c", &octets[0], &octets[1], &octets[2], &octets[3], &trailing) != 4)
            return false;
        for (uint8_t i = 0; i < 4; i++) {
            if (octets[i] > 255) return false;
            address[i] = static_cast<uint8_t>(octets[i]);
        }
        return true;
    }

    bool allowedCommand(const char *command)
    {
        static const char *const exact[] = {
            "SRV ENTER", "SRV EXIT", "mb_30", "mb_31 1", "mb_32 1", "mb_33",
            "mb_61 1", "mb_62 1", "mb_63 1", "mb_64 1", "mb_65 1",
            "mb_66 0", "mb_66 1", "mb_67", "mb_68", "mb_45", "mb_46", "mb_47"
        };
        for (const char *candidate : exact) if (strcmp(command, candidate) == 0) return true;
        if (strncmp(command, "mb_11 ", 6) == 0 || strncmp(command, "mb_12 ", 6) == 0) return true;
        return false;
    }

    // ------------------------------------------------------------------
    // Route handlers
    // ------------------------------------------------------------------

    void handleIndex() { sendFlash(200, "OK", "text/html; charset=utf-8", WEBUI_INDEX_HTML); }
    void handleStylesheet() { sendFlash(200, "OK", "text/css; charset=utf-8", WEBUI_APP_CSS); }
    void handleScript() { sendFlash(200, "OK", "application/javascript; charset=utf-8", WEBUI_APP_JS); }

    const char *directionName(Direction value)
    {
        switch (value) {
            case Direction::Up: return "UP";
            case Direction::Down: return "DOWN";
            default: return "IDLE";
        }
    }

    const char *serviceName(ServiceState value)
    {
        switch (value) {
            case ServiceState::Fault: return "FAULT";
            case ServiceState::Revision: return "REVISION";
            case ServiceState::Missing: return "MISSING";
            default: return "NORMAL";
        }
    }

    void handleState()
    {
        SharedPanel panel{};
        if (!ApplicationState::read(panel)) {
            sendText(503, "Service Unavailable", "Snapshot indisponibil");
            return;
        }
        const IPAddress ip = Ethernet.localIP();
        const Settings::NetworkSettings network = ConfigurationService::networkSettings();
        UpdateService::Status updateStatus;
        UpdateService::getStatus(updateStatus);
        // newlib-nano nu suporta %f; formatare manuala cu doua zecimale.
        const float gain = ConfigurationService::audioGain();
        const unsigned gainCents = static_cast<unsigned>(gain * 100.0f + 0.5f);
        char json[1152];
        const int length = snprintf(json, sizeof(json),
            "{\"lift\":{\"floor\":%u,\"destination\":%u,\"direction\":\"%s\",\"service\":\"%s\",\"occupied\":%s},"
            "\"system\":{\"uptime\":%lu,\"resetReason\":%u},\"diagnostics\":{\"frames\":%lu,\"valid\":%lu,\"timeouts\":%lu,\"format\":%lu,\"crc\":%lu,\"data\":%lu,\"counting\":%s},"
            "\"network\":{\"ip\":\"%u.%u.%u.%u\",\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"dhcp\":%s,"
            "\"staticIp\":\"%u.%u.%u.%u\",\"subnet\":\"%u.%u.%u.%u\","
            "\"gateway\":\"%u.%u.%u.%u\",\"dns\":\"%u.%u.%u.%u\"},"
            "\"config\":{\"lift\":%u,\"gain\":%u.%02u,\"auth\":%s,\"serviceMode\":%s},"
            "\"update\":{\"phase\":%u,\"bytes\":%lu,\"total\":%lu,\"error\":\"%s\",\"file\":\"%s\","
            "\"compatKnown\":%s,\"compat\":%s,\"bootOk\":%s}}",
            static_cast<unsigned>(panel.lift1.pos), static_cast<unsigned>(panel.lift1.etd),
            directionName(panel.lift1.sj), serviceName(panel.lift1.svc),
            panel.lift1.ocp == Occupancy::Busy ? "true" : "false",
            static_cast<unsigned long>(panel.system.uptimeSeconds),
            static_cast<unsigned>(panel.system.lastResetReason),
            static_cast<unsigned long>(panel.comm.lift1.rxFrames),
            static_cast<unsigned long>(panel.comm.lift1.rxValid),
            static_cast<unsigned long>(panel.comm.lift1.rxTimeout),
            static_cast<unsigned long>(panel.comm.lift1.rxFormatError),
            static_cast<unsigned long>(panel.comm.lift1.rxCrcError),
            static_cast<unsigned long>(panel.comm.lift1.rxDataError),
            panel.comm.countingEnabled ? "true" : "false",
            ip[0], ip[1], ip[2], ip[3],
            Config::Ethernet::MAC[0], Config::Ethernet::MAC[1], Config::Ethernet::MAC[2],
            Config::Ethernet::MAC[3], Config::Ethernet::MAC[4], Config::Ethernet::MAC[5],
            network.dhcp != 0 ? "true" : "false",
            network.ip[0], network.ip[1], network.ip[2], network.ip[3],
            network.subnet[0], network.subnet[1], network.subnet[2], network.subnet[3],
            network.gateway[0], network.gateway[1], network.gateway[2], network.gateway[3],
            network.dns[0], network.dns[1], network.dns[2], network.dns[3],
            static_cast<unsigned>(ConfigurationService::liftNumber()),
            gainCents / 100, gainCents % 100,
            Config::Web::AUTH_PASSWORD[0] != '\0' ? "true" : "false",
            ServiceMenu::isInServiceMode() ? "true" : "false",
            static_cast<unsigned>(updateStatus.phase),
            static_cast<unsigned long>(updateStatus.bytes),
            static_cast<unsigned long>(updateStatus.total),
            updateStatus.error, updateStatus.fileName,
            updateStatus.compatibilityKnown ? "true" : "false",
            updateStatus.compatible ? "true" : "false",
            updateStatus.bootConfirmed ? "true" : "false");
        if (length <= 0 || static_cast<size_t>(length) >= sizeof(json)) {
            sendText(500, "Internal Server Error", "Eroare serializare stare");
            return;
        }
        sendResponse(200, "OK", "application/json; charset=utf-8", json, static_cast<size_t>(length), true);
    }

    void handleAuth()
    {
        if (!requireSameOrigin()) return;
        if (!requirePlainTextBody()) return;
        terminateBody();
        if (!requireAuth()) return;
        if (strcmp(sRequest.body, "AUTH") != 0) {
            sendText(400, "Bad Request", "Corp de cerere neasteptat");
            return;
        }
        sendText(200, "OK", "Autentificare reusita");
    }

    // Corpul upload-ului UF2 este consumat ca stream; aici ajung doar
    // cererile fara Content-Length.
    void handleUploadNoBody()
    {
        sendText(400, "Bad Request", "Lipseste continutul UF2");
    }

    void handleService()
    {
        if (!requireAuth()) return;
        if (!requireSameOrigin()) return;
        if (!requirePlainTextBody()) return;
        terminateBody();
        if (!allowedCommand(sRequest.body)) {
            sendText(400, "Bad Request", "Comanda nepermisa");
            return;
        }
        sServiceResponse[0] = '\0';
        sCapturingResponse = true;
        ServiceProtocol::submitLine(sRequest.body);
        sCapturingResponse = false;
        if (sServiceResponse[0] != '\0') {
            char result[112];
            const int length = snprintf(result, sizeof(result), "{\"response\":\"%s\"}", sServiceResponse);
            if (length > 0 && static_cast<size_t>(length) < sizeof(result)) {
                sendResponse(200, "OK", "application/json; charset=utf-8", result, static_cast<size_t>(length), true);
            } else {
                sendText(500, "Internal Server Error", "Raspuns prea lung");
            }
        } else if (ServiceProtocol::hasPendingCommand()) {
            sendText(202, "Accepted", "Comanda acceptata");
        } else {
            sendText(503, "Service Unavailable", "Fara raspuns de la dispatcher");
        }
    }

    // Pornirea stream-ului de upload UF2, apelata imediat dupa headere.
    // Toate verificarile de securitate ruleaza inainte de primirea corpului.
    void beginUploadStream()
    {
        if (!requireAuth() || !requireSameOrigin()) return;
        if (!ServiceMenu::isInServiceMode()) {
            sendText(403, "Forbidden", "Intrati in Service Mode pentru upload firmware");
            return;
        }
        if (strncmp(sRequest.contentType, "application/octet-stream", 24) != 0) {
            sendText(415, "Unsupported Media Type", "Trimiteti fisierul ca application/octet-stream");
            return;
        }
        if (!UpdateService::beginUpload(sRequest.contentLength, sRequest.uploadFileName)) {
            sendText(409, "Conflict", UpdateService::lastError());
            return;
        }
    }

    void handleUpdateVerify()
    {
        if (!requireAuth() || !requireSameOrigin() || !requirePlainTextBody()) return;
        terminateBody();
        if (!ServiceMenu::isInServiceMode()) {
            sendText(403, "Forbidden", "Intrati in Service Mode pentru verificare");
            return;
        }
        if (strcmp(sRequest.body, "VERIFY") != 0) {
            sendText(400, "Bad Request", "Corp de cerere neasteptat");
            return;
        }
        if (!UpdateService::startVerify()) {
            sendText(409, "Conflict", UpdateService::lastError());
            return;
        }
        sendText(200, "OK", "Verificare pornita");
    }

    void handleUpdateApply()
    {
        if (!requireAuth() || !requireSameOrigin() || !requirePlainTextBody()) return;
        terminateBody();
        if (strcmp(sRequest.body, "APPLY") != 0) {
            sendText(400, "Bad Request", "Corp de cerere neasteptat");
            return;
        }
        if (!UpdateService::requestApply()) {
            sendText(409, "Conflict", UpdateService::lastError());
            return;
        }
        sendText(200, "OK", "Instalare pornita; panoul va reporni");
    }

    void handleUpdateCancel()
    {
        if (!requireAuth() || !requireSameOrigin() || !requirePlainTextBody()) return;
        terminateBody();
        if (strcmp(sRequest.body, "CANCEL") != 0) {
            sendText(400, "Bad Request", "Corp de cerere neasteptat");
            return;
        }
        UpdateService::cancel();
        sendText(200, "OK", "Update abandonat");
    }

    void handleNetwork()
    {
        if (!requireAuth()) return;
        if (!requireSameOrigin()) return;
        if (!requirePlainTextBody()) return;
        if (!ServiceMenu::isInServiceMode()) {
            sendText(403, "Forbidden", "Intrati in Service Mode pentru configurare");
            return;
        }
        terminateBody();
        Settings::NetworkSettings settings = ConfigurationService::networkSettings();
        if (strcmp(sRequest.body, "DHCP") == 0) {
            settings.dhcp = 1;
        } else {
            char ipText[16], subnetText[16], gatewayText[16], dnsText[16], trailing;
            const int fields = sscanf(sRequest.body, "STATIC %15s %15s %15s %15s %c",
                                      ipText, subnetText, gatewayText, dnsText, &trailing);
            if (fields != 4 || !parseIPv4(ipText, settings.ip) || !parseIPv4(subnetText, settings.subnet) ||
                !parseIPv4(gatewayText, settings.gateway) || !parseIPv4(dnsText, settings.dns)) {
                sendText(400, "Bad Request", "Configuratie IP invalida");
                return;
            }
            settings.dhcp = 0;
        }
        if (!ConfigurationService::setNetworkSettings(settings)) {
            sendText(400, "Bad Request", "Adresele sau masca nu sunt valide");
            return;
        }
        sendText(200, "OK", "Configuratie salvata; reporniti panoul pentru aplicare");
    }

    // ------------------------------------------------------------------
    // Routing
    // ------------------------------------------------------------------

    struct Route
    {
        const char *method;
        const char *path;
        void (*handler)();
    };

    const Route ROUTES[] = {
        { "GET",  "/",            handleIndex },
        { "GET",  "/app.css",     handleStylesheet },
        { "GET",  "/app.js",      handleScript },
        { "GET",  "/api/state",   handleState },
        { "POST", "/api/auth",    handleAuth },
        { "POST", "/api/service", handleService },
        { "POST", "/api/network", handleNetwork },
        { "POST", "/api/update/upload", handleUploadNoBody },
        { "POST", "/api/update/verify", handleUpdateVerify },
        { "POST", "/api/update/apply",  handleUpdateApply },
        { "POST", "/api/update/cancel", handleUpdateCancel },
    };

    void handleRequest()
    {
        for (const Route &route : ROUTES) {
            if (strcmp(sRequest.method, route.method) == 0 && strcmp(sRequest.path, route.path) == 0) {
                route.handler();
                return;
            }
        }
        sendText(404, "Not Found", "Ruta inexistenta");
    }

    // ------------------------------------------------------------------
    // HTTP parsing (one client at a time, line-based headers)
    // ------------------------------------------------------------------

    void copyHeaderValue(char *destination, size_t capacity, const char *source)
    {
        if (capacity == 0) return;
        size_t length = 0;
        while (length + 1 < capacity && source[length] != '\0') {
            destination[length] = source[length];
            length++;
        }
        destination[length] = '\0';
    }

    void processLine()
    {
        sRequest.line[sRequest.lineLength] = '\0';
        if (sRequest.lineLength == 0) {
            sRequest.headersDone = true;
            if (sRequest.isUploadStream) {
                beginUploadStream();
            } else if (sRequest.contentLength == 0) {
                handleRequest();
            }
        } else if (sRequest.method[0] == '\0') {
            if (sscanf(sRequest.line, "%7s %95s", sRequest.method, sRequest.path) != 2) sRequest.overflow = true;
        } else if (strncasecmp(sRequest.line, "Authorization: Basic ", 21) == 0) {
            copyHeaderValue(sRequest.authorization, sizeof(sRequest.authorization), sRequest.line + 21);
        } else if (strncasecmp(sRequest.line, "Host: ", 6) == 0) {
            copyHeaderValue(sRequest.host, sizeof(sRequest.host), sRequest.line + 6);
        } else if (strncasecmp(sRequest.line, "Origin: ", 8) == 0) {
            copyHeaderValue(sRequest.origin, sizeof(sRequest.origin), sRequest.line + 8);
        } else if (strncasecmp(sRequest.line, "Content-Type: ", 14) == 0) {
            copyHeaderValue(sRequest.contentType, sizeof(sRequest.contentType), sRequest.line + 14);
        } else if (strncasecmp(sRequest.line, "X-Filename: ", 12) == 0) {
            copyHeaderValue(sRequest.uploadFileName, sizeof(sRequest.uploadFileName), sRequest.line + 12);
        } else if (strncasecmp(sRequest.line, "Content-Length: ", 16) == 0) {
            const unsigned long length = strtoul(sRequest.line + 16, nullptr, 10);
            const bool uploadRequest = strcmp(sRequest.method, "POST") == 0 &&
                                       strcmp(sRequest.path, "/api/update/upload") == 0;
            if (uploadRequest) {
                sRequest.contentLength = static_cast<size_t>(length);
                sRequest.isUploadStream = true;
            } else if (length >= BODY_SIZE) {
                sRequest.overflow = true;
            } else {
                sRequest.contentLength = static_cast<size_t>(length);
            }
        }
        sRequest.lineLength = 0;
    }

    void acceptRequest()
    {
        EthernetClient incoming = sServer.accept();
        if (!incoming) return;
        resetRequest();
        sRequest.client = incoming;
        sRequest.active = true;
        sRequest.lastByteAt = millis();
    }

    void pollRequest()
    {
        if (!sRequest.active) return;
        if (!sRequest.client.connected() && sRequest.client.available() == 0) {
            resetRequest();
            return;
        }
        const uint32_t timeout = sRequest.isUploadStream ? UPLOAD_TIMEOUT_MS : REQUEST_TIMEOUT_MS;
        if (millis() - sRequest.lastByteAt > timeout) {
            if (sRequest.isUploadStream) UpdateService::abortUpload();
            sendText(408, "Request Timeout", "Timeout");
            return;
        }
        uint8_t budget = sRequest.isUploadStream ? UPLOAD_CHUNK_BUDGET : POLL_BYTE_BUDGET;
        while (budget-- > 0 && sRequest.client.available() > 0) {
            if (sRequest.isUploadStream && sRequest.headersDone) {
                // Stream binar catre UpdateService (SD), in tranșe, fara
                // a stoca firmware-ul in RAM.
                uint8_t chunk[512];
                const size_t remaining = sRequest.contentLength - sRequest.bodyLength;
                const size_t wanted = remaining < sizeof(chunk) ? remaining : sizeof(chunk);
                const int got = sRequest.client.read(chunk, wanted);
                if (got <= 0) break;
                sRequest.lastByteAt = millis();
                if (!UpdateService::writeChunk(chunk, static_cast<size_t>(got))) {
                    UpdateService::abortUpload();
                    sendText(500, "Internal Server Error", "Eroare scriere pe cardul SD");
                    return;
                }
                sRequest.bodyLength += static_cast<size_t>(got);
                if (sRequest.bodyLength == sRequest.contentLength) {
                    if (UpdateService::endUpload()) {
                        sendText(200, "OK", "Upload complet; rulati Verificare");
                    } else {
                        sendText(500, "Internal Server Error", UpdateService::lastError());
                    }
                    return;
                }
                continue;
            }
            const int next = sRequest.client.read();
            if (next < 0) break;
            sRequest.lastByteAt = millis();
            const char value = static_cast<char>(next);
            if (sRequest.overflow) {
                sendText(413, "Content Too Large", "Cerere prea mare");
                return;
            }
            if (!sRequest.headersDone) {
                if (value == '\r') continue;
                if (value == '\n') processLine();
                else if (sRequest.lineLength + 1 < sizeof(sRequest.line)) sRequest.line[sRequest.lineLength++] = value;
                else sRequest.overflow = true;
            } else {
                if (sRequest.bodyLength >= sizeof(sRequest.body) - 1) {
                    sendText(413, "Content Too Large", "Cerere prea mare");
                    return;
                }
                sRequest.body[sRequest.bodyLength++] = value;
                if (sRequest.bodyLength == sRequest.contentLength) {
                    handleRequest();
                    return;
                }
            }
            if (!sRequest.active) return;
        }
    }
}

void begin()
{
    sServer.begin();
}

void poll()
{
    if (!sRequest.active) acceptRequest();
    pollRequest();
}

void captureServiceResponse(const char *response)
{
    if (!sCapturingResponse || response == nullptr) return;
    snprintf(sServiceResponse, sizeof(sServiceResponse), "%s", response);
}
}

#endif
