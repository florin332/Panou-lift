#ifndef UI_THEME_H
#define UI_THEME_H

#include <string.h>

namespace UITheme
{
    // Returnează traducerea umană curată direct pe baza cheii abstracte din model
    inline const char* getText(const char* labelKey) {
        if (labelKey == NULL) return "";

        // --- TITLURI DE DASHBOARD ---
        if (strcmp(labelKey, "SYS_STATUS_TITLE") == 0) return "1. STATUS PANOU";
        if (strcmp(labelKey, "LIFT_1_TITLE") == 0)     return "2. DASHBOARD LIFT 1";
        if (strcmp(labelKey, "LIFT_2_TITLE") == 0)     return "3. DASHBOARD LIFT 2";
        if (strcmp(labelKey, "COMM_TITLE") == 0)       return "4. COMUNICATIE";
        if (strcmp(labelKey, "VER_TITLE") == 0)        return "5. DATE PRODUCTIE";
        
        if (strcmp(labelKey, "DEV_ENG_TITLE") == 0)    return "ENG. PROPERTIES";
        if (strcmp(labelKey, "DEV_MEM_TITLE") == 0)    return "MEMORY LAYOUT RAM";
        if (strcmp(labelKey, "DEV_TSK_TITLE") == 0)    return "TASKS REALTIME";
        if (strcmp(labelKey, "DEV_PROT_TITLE") == 0)   return "PROTOCOL TELEMETRY";
        if (strcmp(labelKey, "DEV_ASRT_TITLE") == 0)   return "ASSERTIONS CHECK";

        // --- ETICHETE DE PARAMETRII ---
        if (strcmp(labelKey, "SYS_BOOT_CNT") == 0)     return "Contor Porniri:";
        if (strcmp(labelKey, "SYS_RST_REAS") == 0)     return "Ultimul Reset :";
        if (strcmp(labelKey, "SYS_UPTIME") == 0)       return "Uptime       :";
        if (strcmp(labelKey, "LIFT_POS") == 0)         return "Etaj Curent   :";
        if (strcmp(labelKey, "LIFT_TRG") == 0)         return "Etaj Comandat :";
        if (strcmp(labelKey, "LIFT_DIR") == 0)         return "Sens De Mers  :";
        if (strcmp(labelKey, "LIFT_SVC") == 0)         return "Regim Lucru   :";
        if (strcmp(labelKey, "LIFT_OCP") == 0)         return "Stare Cabina  :";
        
        if (strcmp(labelKey, "COMM_L1_ERR") == 0)      return "Pierderi Pct L1:";
        if (strcmp(labelKey, "COMM_L2_ERR") == 0)      return "Pierderi Pct L2:";
        if (strcmp(labelKey, "COMM_MISS_PKT") == 0)    return "Pachete Pierd.:";
        if (strcmp(labelKey, "COMM_TIMEOUTS") == 0)    return "Timeouts     :";
        
        if (strcmp(labelKey, "VER_FIRM") == 0)         return "Versiune Soft :";
        if (strcmp(labelKey, "VER_BUILD") == 0)        return "Cod Compilare :";

        if (strcmp(labelKey, "SIZEOF_PANEL") == 0)     return "sizeof(Panel) :";
        if (strcmp(labelKey, "SIZEOF_SYSTEM") == 0)    return "sizeof(System):";
        if (strcmp(labelKey, "SIZEOF_UI") == 0)        return "sizeof(UI)   :";
        if (strcmp(labelKey, "SIZEOF_LIFT") == 0)      return "sizeof(Lift) :";
        if (strcmp(labelKey, "RAM_ADDR_MAP") == 0)     return "Shared RAM A :";
        if (strcmp(labelKey, "SEQLOCK_VAL") == 0)      return "Seqlock Value:";
        if (strcmp(labelKey, "RAM_ALIGN") == 0)        return "RAM Alignment:";
        if (strcmp(labelKey, "SEQLOCK_COLL") == 0)     return "Collisions   :";
        
        if (strcmp(labelKey, "CORE0_PROC") == 0)       return "Core0 Process:";
        if (strcmp(labelKey, "CORE1_PROC") == 0)       return "Core1 Process:";
        if (strcmp(labelKey, "LOOP0_JITT") == 0)       return "Loop0 Tact Ms:";
        if (strcmp(labelKey, "LOOP1_TACT") == 0)       return "Loop1 Tact Us:";
        
        if (strcmp(labelKey, "UART1_RX_PKT") == 0)     return "UART1 RX Pack:";
        if (strcmp(labelKey, "UART2_RX_PKT") == 0)     return "UART2 RX Pack:";
        if (strcmp(labelKey, "UART_TX_PKT") == 0)      return "TX Call Pack :";
        if (strcmp(labelKey, "PARSER_ERRS") == 0)      return "Parser Errors:";
        if (strcmp(labelKey, "CRC_FAILURES") == 0)     return "CRC Failures :";
        
        if (strcmp(labelKey, "ASRT_PANEL") == 0)       return "SharedPanel  :";
        if (strcmp(labelKey, "ASRT_EEPROM") == 0)      return "EEPROM sector:";
        if (strcmp(labelKey, "ASRT_UART1") == 0)       return "UART1 Hardware:";
        if (strcmp(labelKey, "ASRT_UART2") == 0)       return "UART2 Hardware:";
        if (strcmp(labelKey, "ASRT_SPI_BUS") == 0)     return "Display SPI  :";

        return labelKey; // Fallback sigurs binar
    }
}

#endif // UI_THEME_H
