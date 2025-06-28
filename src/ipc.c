#include <swilib.h>
#include "ipc.h"

const char *IPC_NAME_XTASK = IPC_XTASK_NAME;

void IPC_SendToXTask(IPC_REQ *ipc, int submess) {
    ipc->name_to = IPC_NAME_XTASK;
    ipc->name_from = IPC_NAME_XTASK;
    GBS_SendMessage(MMI_CEPID, MSG_IPC, submess, ipc);
}
