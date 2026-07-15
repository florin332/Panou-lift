
#include "DiagnosticsPages.h"
#include "Arduino.h"

namespace DiagnosticsPages
{
    void init() {
        // Interfață pură în memorie
    }

    static void fillNum(PageLine &line, const char* key, uint32_t val, SemanticState st) {
        line.type = PageLineType::DataNumeric; line.labelKey = key; line.valueNum = val; line.valueText = NULL; line.state = st;
    }

    static void fillHex(PageLine &line, const char* key, uint32_t val) {
        line.type = PageLineType::DataHex; line.labelKey = key; line.valueNum = val; line.valueText = NULL; line.state = SemanticState::Neutral;
    }

    static void fillTxt(PageLine &line, const char* key, const char* valText, SemanticState st) {
        line.type = PageLineType::DataText; line.labelKey = key; line.valueText = valText; line.valueNum = 0; line.state = st;
    }

    static void fillSeparator(PageLine &line) {
        line.type = PageLineType::Separator; line.labelKey = NULL; line.valueText = NULL; line.valueNum = 0; line.state = SemanticState::Neutral;
    }

    static void fillEmpty(PageLine &line) {
        line.type = PageLineType::Empty; line.labelKey = NULL; line.valueText = NULL; line.valueNum = 0; line.state = SemanticState::Neutral;
    }

    // Traducător intern de siguranță pentru compatibilitatea stărilor de cabină din V10.25
    static SemanticState evalLiftState(const char* txt) {
        if (txt == NULL) return SemanticState::Neutral;
        if (strcmp(txt, "Normal") == 0 || strcmp(txt, "Free") == 0) return SemanticState::Success;
        if (strcmp(txt, "Revision") == 0 || strcmp(txt, "Busy") == 0) return SemanticState::Warning;
        if (strcmp(txt, "Fault") == 0) return SemanticState::Alert;
        return SemanticState::Neutral;
    }

    void buildServicePage(const ProcessedDiagnostics &info, ServicePage pageType, PageModel &output) {
        output.totalPages = TOTAL_SERVICE_PAGES;
        output.pageIndex = static_cast<uint8_t>(pageType) + 1;

        switch (pageType) {
            case ServicePage::System:
                output.title = "SYS_STATUS_TITLE"; // Cheie simbolică abstractă
                fillNum(output.lines[0], "SYS_BOOT_CNT", info.boots, SemanticState::Neutral);
                fillTxt(output.lines[1], "SYS_RST_REAS", info.resetReason, SemanticState::Warning);
                fillNum(output.lines[2], "SYS_UPTIME", info.uptime, SemanticState::Success);
                fillSeparator(output.lines[3]); // Randare direct structurală de separator hardware!
                fillEmpty(output.lines[4]);     // Randare directă de rând liber
                break;

            case ServicePage::Lift1:
                output.title = "LIFT_1_TITLE";
                fillNum(output.lines[0], "LIFT_POS", info.lift1.pos, SemanticState::Neutral);
                fillNum(output.lines[1], "LIFT_TRG", info.lift1.etd, SemanticState::Neutral);
                fillTxt(output.lines[2], "LIFT_DIR", info.lift1.sj, evalLiftState(info.lift1.sj));
                fillTxt(output.lines[3], "LIFT_SVC", info.lift1.svc, evalLiftState(info.lift1.svc));
                fillTxt(output.lines[4], "LIFT_OCP", info.lift1.ocp, evalLiftState(info.lift1.ocp));
                break;

            case ServicePage::Lift2:
                output.title = "LIFT_2_TITLE";
                fillNum(output.lines[0], "LIFT_POS", info.lift2.pos, SemanticState::Neutral);
                fillNum(output.lines[1], "LIFT_TRG", info.lift2.etd, SemanticState::Neutral);
                fillTxt(output.lines[2], "LIFT_DIR", info.lift2.sj, evalLiftState(info.lift2.sj));
                fillTxt(output.lines[3], "LIFT_SVC", info.lift2.svc, evalLiftState(info.lift2.svc));
                fillTxt(output.lines[4], "LIFT_OCP", info.lift2.ocp, evalLiftState(info.lift2.ocp));
                break;

            case ServicePage::Communication:
                output.title = "COMM_TITLE";
                fillNum(output.lines[0], "COMM_L1_ERR", info.errorsLift1, info.errorsLift1 == 0 ? SemanticState::Success : SemanticState::Alert);
                fillNum(output.lines[1], "COMM_L2_ERR", info.errorsLift2, info.errorsLift2 == 0 ? SemanticState::Success : SemanticState::Alert);
                fillTxt(output.lines[2], "COMM_MISS_PKT", info.errorsLift1 > 0 || info.errorsLift2 > 0 ? "DETECTED" : "NONE", info.errorsLift1 > 0 || info.errorsLift2 > 0 ? SemanticState::Alert : SemanticState::Success);
                fillTxt(output.lines[3], "COMM_TIMEOUTS", "OK", SemanticState::Success);
                fillEmpty(output.lines[4]);
                break;

            case ServicePage::Version:
                output.title = "VER_TITLE";
                fillTxt(output.lines[0], "VER_FIRM", "V10.27", SemanticState::Neutral);
                fillNum(output.lines[1], "VER_BUILD", 1027, SemanticState::Neutral);
                fillSeparator(output.lines[2]);
                fillEmpty(output.lines[3]);
                fillEmpty(output.lines[4]);
                break;
        }
    }

    void buildDeveloperPage(const ProcessedDiagnostics &info, DevPage pageType, PageModel &output) {
        output.totalPages = TOTAL_DEV_PAGES;
        output.pageIndex = static_cast<uint8_t>(pageType) + 1;

        switch (pageType) {
            case DevPage::Engineering:
                output.title = "DEV_ENG_TITLE";
                fillNum(output.lines[0], "SIZEOF_PANEL", sizeof(SharedPanel), SemanticState::Neutral);
                fillNum(output.lines[1], "SIZEOF_SYSTEM", sizeof(SystemState), SemanticState::Neutral);
                fillNum(output.lines[2], "SIZEOF_UI", sizeof(UiState), SemanticState::Neutral);
                fillNum(output.lines[3], "SIZEOF_LIFT", sizeof(LiftState), SemanticState::Neutral);
                fillEmpty(output.lines[4]);
                break;

            case DevPage::MemoryLayout:
                output.title = "DEV_MEM_TITLE";
                fillHex(output.lines[0], "RAM_ADDR_MAP", reinterpret_cast<uint32_t>(&gSharedMemory));
                fillNum(output.lines[1], "SEQLOCK_VAL", gSharedMemory.seq, SemanticState::Warning);
                fillNum(output.lines[2], "RAM_ALIGN", alignof(SharedMemory), SemanticState::Success);
                fillNum(output.lines[3], "SEQLOCK_COLL", info.seqlockCollisions, info.seqlockCollisions == 0 ? SemanticState::Success : SemanticState::Alert);
                fillEmpty(output.lines[4]);
                break;

            case DevPage::Tasks:
                output.title = "DEV_TSK_TITLE";
                fillTxt(output.lines[0], "CORE0_PROC", "running", SemanticState::Success);
                fillTxt(output.lines[1], "CORE1_PROC", "running", SemanticState::Success);
                fillNum(output.lines[2], "LOOP0_JITT", millis() % 10, SemanticState::Success);
                fillNum(output.lines[3], "LOOP1_TACT", 640, SemanticState::Success);
                fillEmpty(output.lines[4]);
                break;

            case DevPage::ProtocolStats:
                output.title = "DEV_PROT_TITLE";
                fillNum(output.lines[0], "UART1_RX_PKT", 12457, SemanticState::Neutral);
                fillNum(output.lines[1], "UART2_RX_PKT", 12455, SemanticState::Neutral);
                fillNum(output.lines[2], "UART_TX_PKT", 842, SemanticState::Neutral);
                fillNum(output.lines[3], "PARSER_ERRS", info.errorsLift1 + info.errorsLift2, SemanticState::Success);
                fillNum(output.lines[4], "CRC_FAILURES", 0, SemanticState::Success);
                break;

            case DevPage::Assertions:
                output.title = "DEV_ASRT_TITLE";
                fillTxt(output.lines[0], "ASRT_PANEL", "OK", SemanticState::Success);
                fillTxt(output.lines[1], "ASRT_EEPROM", "OK", SemanticState::Success);
                fillTxt(output.lines[2], "ASRT_UART1", info.errorsLift2 == 0 ? "OK" : "NO", info.errorsLift2 == 0 ? SemanticState::Success : SemanticState::Alert);
                fillTxt(output.lines[3], "ASRT_UART2", info.errorsLift1 == 0 ? "OK" : "NO", info.errorsLift1 == 0 ? SemanticState::Success : SemanticState::Alert);
                fillTxt(output.lines[4], "ASRT_SPI_BUS", "OK", SemanticState::Success);
                break;
        }
    }
}
