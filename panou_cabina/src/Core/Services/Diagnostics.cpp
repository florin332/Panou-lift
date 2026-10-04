#include "Diagnostics.h"

namespace Diagnostics
{
    void init() {
        // Implementare vidă: independență totală de periferice
    }

    static const char* getResetReasonText(uint8_t reason) {
        switch (reason) {
            case 1:  return "POR (Power-On Reset)";
            case 2:  return "Watchdog Timer Reset";
            case 3:  return "Software Reset / RUN Pin";
            default: return "Unknown Reset Reason";
        }
    }

    static const char* getDirectionText(Direction sj) {
        switch (sj) {
            case Direction::Idle: return "Idle";
            case Direction::Up:   return "Up";
            case Direction::Down: return "Down";
            default:              return "N/A";
        }
    }

    static const char* getOccupancyText(Occupancy ocp) {
        switch (ocp) {
            case Occupancy::Free: return "Free";
            case Occupancy::Busy: return "Busy";
            default:              return "N/A";
        }
    }

    static const char* getServiceText(ServiceState svc) {
        switch (svc) {
            case ServiceState::Normal:   return "Normal";
            case ServiceState::Fault:    return "Fault";
            case ServiceState::Revision: return "Revision";
            case ServiceState::Missing:  return "No Serial";
            default:                     return "Unknown";
        }
    }

    void interpret(const SharedPanel &snapshot, ProcessedDiagnostics &output) {
        // 1. Traducere metrici sistem
        output.boots = snapshot.system.bootCounter;
        output.resetReason = getResetReasonText(snapshot.system.lastResetReason);
        output.uptime = snapshot.system.uptimeSeconds;

        // 2. Mapare ierarhizată directă pentru Cabina 1 (lift1. ...)
        output.lift1.pos = snapshot.lift1.pos;
        output.lift1.etd = snapshot.lift1.etd;
        output.lift1.sj  = getDirectionText(snapshot.lift1.sj);
        output.lift1.ocp = getOccupancyText(snapshot.lift1.ocp);
        output.lift1.svc = getServiceText(snapshot.lift1.svc);

        // 3. Copiere statistici sistem
        output.seqlockCollisions = snapshot.system.seqlockCollisions;
        output.rxFrames = snapshot.comm.lift1.rxFrames;
        output.rxTimeouts = snapshot.comm.lift1.rxTimeout;
        output.rxFormatErrors = snapshot.comm.lift1.rxFormatError;
        output.rxCrcErrors = snapshot.comm.lift1.rxCrcError;
        output.rxDataErrors = snapshot.comm.lift1.rxDataError;
    }
}
