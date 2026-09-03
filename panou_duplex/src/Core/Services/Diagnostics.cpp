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

        // 3. Mapare ierarhizată directă pentru Cabina 2 (lift2. ...)
        output.lift2.pos = snapshot.lift2.pos;
        output.lift2.etd = snapshot.lift2.etd;
        output.lift2.sj  = getDirectionText(snapshot.lift2.sj);
        output.lift2.ocp = getOccupancyText(snapshot.lift2.ocp);
        output.lift2.svc = getServiceText(snapshot.lift2.svc);

        // 4. Copiere statistici magistrală
        output.errorsLift1 = snapshot.system.packetErrorsLift1;
        output.errorsLift2 = snapshot.system.packetErrorsLift2;
        output.seqlockCollisions = snapshot.system.seqlockCollisions;
    }
}
