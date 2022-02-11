/*
  Copyright (c) 2019-22 John MacCallum Permission is hereby granted,
  free of charge, to any person obtaining a copy of this software
  and associated documentation files (the "Software"), to deal in
  the Software without restriction, including without limitation the
  rights to use, copy, modify, merge, publish, distribute,
  sublicense, and/or sell copies of the Software, and to permit
  persons to whom the Software is furnished to do so, subject to the
  following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*/

#include "ose_conf.h"
#include "ose.h"
#include "ose_context.h"
#include "ose_util.h"
#include "ose_stackops.h"
#include "ose_assert.h"
#include "ose_vm.h"

static void ose_db_get(ose_bundle osevm, int32_t n)
{
    ose_bundle vm_db = ose_enter(osevm, "/db");
    ose_bundle vm_s = OSEVM_STACK(osevm);
    int32_t o = OSE_BUNDLE_HEADER_LEN;
    int32_t i = 0;
    for(i = 0; i < n; i++)
    {
        o += ose_readInt32(vm_db, o) + 4;
    }
    ose_copyElemAtOffset(o, vm_db, vm_s);
}

static void ose_db_getInput(ose_bundle osevm)
{
    ose_db_get(osevm, 1);
}

static void ose_db_getStack(ose_bundle osevm)
{
    ose_db_get(osevm, 4);
}

static void ose_db_getEnv(ose_bundle osevm)
{
    ose_db_get(osevm, 2);
}

static void ose_db_getControl(ose_bundle osevm)
{
    ose_db_get(osevm, 3);
}

static void ose_db_getDump(ose_bundle osevm)
{
    ose_db_get(osevm, 0);
}

static void ose_db_set(ose_bundle osevm, int32_t n)
{
    ose_bundle vm_db = ose_enter(osevm, "/db");
    ose_bundle vm_s = OSEVM_STACK(osevm);
    ose_pushInt32(vm_db, n);
    ose_roll(vm_db);
    ose_drop(vm_db);
    ose_pushInt32(vm_db, n);
    ose_bundleFromTop(vm_db);
    ose_moveElem(vm_s, vm_db);
    ose_swap(vm_db);
    ose_unpackDrop(vm_db);
}

static void ose_db_setInput(ose_bundle osevm)
{
    ose_db_set(osevm, 3);
}

static void ose_db_setStack(ose_bundle osevm)
{
    ose_db_set(osevm, 0);
}

static void ose_db_setEnv(ose_bundle osevm)
{
    ose_db_set(osevm, 2);
}

static void ose_db_setControl(ose_bundle osevm)
{
    ose_db_set(osevm, 1);
}

static void ose_db_setDump(ose_bundle osevm)
{
    ose_db_set(osevm, 4);
}

static void ose_db_debug(ose_bundle osevm)
{
    ose_bundle vm_i = OSEVM_INPUT(osevm);
    ose_bundle vm_s = OSEVM_STACK(osevm);
    ose_bundle vm_e = OSEVM_ENV(osevm);
    ose_bundle vm_c = OSEVM_CONTROL(osevm);
    ose_bundle vm_d = OSEVM_DUMP(osevm);
    ose_bundle vm_db;
    const int32_t freespace = ose_spaceAvailable(osevm);

    int32_t db_size = 0;
    int32_t s = ose_readSize(vm_i);
    s += ose_readSize(vm_s);
    s += ose_readSize(vm_e);
    s += ose_readSize(vm_c);
    s += ose_readSize(vm_d);
    if(s < freespace)
    {
        if(s * 2 <= freespace)
        {
            db_size = s * 2;
        }
        else
        {
            db_size = freespace - s;
        }
    }
    else
    {
        /* not enough memory for the debugger :( */
        return;
    }
    ose_pushContextMessage(osevm, db_size, "/db");    
    vm_db = ose_enter(osevm, "/db");
    
    ose_copyBundle(vm_d, vm_db);
    ose_clear(vm_d);

    ose_copyBundle(vm_i, vm_db);
    ose_clear(vm_i);

    ose_copyBundle(vm_e, vm_db);
    ose_clear(vm_e);

    ose_drop(vm_c);
    ose_copyBundle(vm_c, vm_db);
    ose_clear(vm_c);
    /* hook for the client to prepare */
    ose_pushString(vm_c, "/!/db/start");
    /* ose_swap(vm_c); */
    /* make sure there's something for the vm to drop */
    ose_pushString(vm_c, "");

    /* ose_bundleAll(vm_s); */
    /* ose_pop(vm_s); */
    /* ose_drop(vm_s); */
    /* ose_pushMessage(vm_db, "/ex", 3, 1, */
    /*                 OSETT_INT32, ose_popInt32(vm_s)); */
    /* ose_moveElem(vm_s, vm_db); */
    ose_copyBundle(vm_s, vm_db);
    ose_clear(vm_s);
}

static void ose_db_enter(ose_bundle osevm)
{
    ose_bundle vm_i = OSEVM_INPUT(osevm);
    ose_bundle vm_s = OSEVM_STACK(osevm);
    ose_bundle vm_e = OSEVM_ENV(osevm);
    ose_bundle vm_c = OSEVM_CONTROL(osevm);
    ose_bundle vm_d = OSEVM_DUMP(osevm);
    ose_bundle vm_db;
    const int32_t freespace = ose_spaceAvailable(osevm);
    /* ose_bundle vm_db = OSEVM_DBUTPUT(osevm); */
    int32_t db_size = 0;
    int32_t s = ose_readSize(vm_i);
    s += ose_readSize(vm_s);
    s += ose_readSize(vm_e);
    s += ose_readSize(vm_c);
    s += ose_readSize(vm_d);
    if(s < freespace)
    {
        if(s * 2 <= freespace)
        {
            db_size = s * 2;
        }
        else
        {
            db_size = freespace - s;
        }
    }
    else
    {
        /* not enough memory for the debugger :( */
        return;
    }
    ose_pushContextMessage(osevm, db_size, "/db");    
    vm_db = ose_enter(osevm, "/db");
    
    
    ose_copyBundle(vm_d, vm_db);
    ose_clear(vm_d);

    ose_copyBundle(vm_i, vm_db);
    ose_clear(vm_i);

    ose_copyBundle(vm_e, vm_db);
    ose_clear(vm_e);

    /* whatever was called to get us here is on top of control.
       drop it, so that the instruction that caused us to get here
       is there instead */
    ose_drop(vm_c);
    ose_copyBundle(vm_c, vm_db);
    ose_clear(vm_c);
    /* hook for the client to prepare */
    ose_pushString(vm_c, "/!/db/start");
    /* make sure there's something for the vm to drop */
    ose_pushString(vm_c, "");

    /* the topmost element of the stack is the exception number.  we
       want to leave it as the only element of the stack once we're
       in the debugger. */
    ose_bundleAll(vm_s);
    ose_pop(vm_s);
    ose_drop(vm_s);
    /* ose_pushMessage(vm_db, "/ex", 3, 1, */
    /*                 OSETT_INT32, ose_popInt32(vm_s)); */
    ose_moveElem(vm_s, vm_db);
}

static void ose_db_exit(ose_bundle osevm)
{
    /* oserepl_debug(osevm); */
    ose_bundle vm_i = OSEVM_INPUT(osevm);
    ose_bundle vm_s = OSEVM_STACK(osevm);
    ose_bundle vm_e = OSEVM_ENV(osevm);
    ose_bundle vm_c = OSEVM_CONTROL(osevm);
    ose_bundle vm_d = OSEVM_DUMP(osevm);
    ose_bundle vm_db = ose_enter(osevm, "/db");
    
    ose_replaceBundle(vm_db, vm_s);
    ose_replaceBundle(vm_db, vm_c);
    /* hook for the client to finalize */
    ose_pushString(vm_c, "/!/db/end");
    /* in dbenter, we removed the message that caused us to enter
       the debugger, so here, we put something back for the vm to
       drop. */
    ose_pushString(vm_c, "");
    ose_replaceBundle(vm_db, vm_e);
    ose_replaceBundle(vm_db, vm_i);
    ose_replaceBundle(vm_db, vm_d);

    ose_dropContextMessage(osevm);
}

static void ose_db_abort(ose_bundle osevm)
{
    ose_dropContextMessage(osevm);
}


#define ose_pushMessageWithAddressLiteral(b, s, n, ...) \
    ose_pushMessage(b, ""s, sizeof(""s) - 1, n, __VA_ARGS__)
void ose_main(ose_bundle osevm)
{
    ose_bundle vm_s = OSEVM_STACK(osevm);
    ose_pushBundle(vm_s);
    ose_pushMessageWithAddressLiteral(vm_s, "/db/enter", 1,
                    OSETT_ALIGNEDPTR, ose_db_enter);
    ose_push(vm_s);
    ose_pushMessageWithAddressLiteral(vm_s, "/db/debug", 1,
                    OSETT_ALIGNEDPTR, ose_db_debug);
    ose_push(vm_s);
    ose_pushMessageWithAddressLiteral(vm_s, "/db/exit", 1,
                    OSETT_ALIGNEDPTR, ose_db_exit);
    ose_push(vm_s);
    ose_pushMessageWithAddressLiteral(vm_s, "/db/abort", 1,
                    OSETT_ALIGNEDPTR, ose_db_abort);
	ose_push(vm_s);
    /* get */
    ose_pushMessageWithAddressLiteral(vm_s, "/db/get/_i", 1,
                    OSETT_ALIGNEDPTR, ose_db_getInput);
    ose_push(vm_s);
    ose_pushMessageWithAddressLiteral(vm_s, "/db/get/_s", 1,
                    OSETT_ALIGNEDPTR, ose_db_getStack);
    ose_push(vm_s);
    ose_pushMessageWithAddressLiteral(vm_s, "/db/get/_e", 1,
                    OSETT_ALIGNEDPTR, ose_db_getEnv);
    ose_push(vm_s);
    ose_pushMessageWithAddressLiteral(vm_s, "/db/get/_c", 1,
                    OSETT_ALIGNEDPTR, ose_db_getControl);
    ose_push(vm_s);
    ose_pushMessageWithAddressLiteral(vm_s, "/db/get/_d", 1,
                    OSETT_ALIGNEDPTR, ose_db_getDump);
    ose_push(vm_s);

    /* set */
    ose_pushMessageWithAddressLiteral(vm_s, "/db/set/_i", 1,
                    OSETT_ALIGNEDPTR, ose_db_setInput);
    ose_push(vm_s);
    ose_pushMessageWithAddressLiteral(vm_s, "/db/set/_s", 1,
                    OSETT_ALIGNEDPTR, ose_db_setStack);
    ose_push(vm_s);
    ose_pushMessageWithAddressLiteral(vm_s, "/db/set/_e", 1,
                    OSETT_ALIGNEDPTR, ose_db_setEnv);
    ose_push(vm_s);
    ose_pushMessageWithAddressLiteral(vm_s, "/db/set/_c", 1,
                    OSETT_ALIGNEDPTR, ose_db_setControl);
    ose_push(vm_s);
    ose_pushMessageWithAddressLiteral(vm_s, "/db/set/_d", 1,
                    OSETT_ALIGNEDPTR, ose_db_setDump);
    ose_push(vm_s);
}
