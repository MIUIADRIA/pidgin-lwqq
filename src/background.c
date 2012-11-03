#include "qq_types.h"

#include <login.h>
#include <async.h>
#include <info.h>
#include <msg.h>
#include <smemory.h>
#include <string.h>

#include "translate.h"

#define START_THREAD(thread,data)\
do{pthread_t th;\
    pthread_create(&th,NULL,thread,data);\
}while(0)


static void* _background_login(void* data)
{
    qq_account* ac=(qq_account*)data;
    if(!qq_account_valid(ac)) return NULL;
    LwqqClient* lc = ac->qq;
    LwqqErrorCode err;
    //it would raise a invalid ac when wake up from sleep.
    //it would login twice,why? 
    //so what i can do is disable the invalid one.
    if(!lwqq_client_valid(lc)) return NULL;
    const char* status = purple_status_get_id(purple_account_get_active_status(ac->account));

    lwqq_login(lc,lwqq_status_from_str(status), &err);

    if (err == LWQQ_EC_LOGIN_NEED_VC) {
        lwqq_async_set_error(lc,VERIFY_COME,err);
        lwqq_async_dispatch(lc,VERIFY_COME,NULL);
    }else{
        lwqq_async_set_error(lc,LOGIN_COMPLETE,err);
        lwqq_async_dispatch(lc,LOGIN_COMPLETE,NULL);
    }
    return NULL;
}


void background_login(qq_account* ac)
{
    START_THREAD(_background_login,ac);
}
static void* _background_friends_info(void* data)
{
    qq_account* ac=(qq_account*)data;
    LwqqErrorCode err;
    LwqqClient* lc=ac->qq;

    LwqqAsyncEvset* lock = lwqq_async_evset_new();
    LwqqAsyncEvent* event;
    event = lwqq_info_get_friends_info(lc,&err);
    lwqq_async_evset_add_event(lock,event);
    event = lwqq_info_get_group_name_list(ac->qq,&err);
    lwqq_async_evset_add_event(lock,event);
    lwqq_async_wait(lock);

    lock = lwqq_async_evset_new();
    event = lwqq_info_get_online_buddies(ac->qq,&err);
    lwqq_async_evset_add_event(lock,event);
    event = lwqq_info_get_discu_name_list(ac->qq);
    lwqq_async_evset_add_event(lock, event);
    lwqq_async_wait(lock);

    lwqq_info_get_friend_detail_info(lc,lc->myself,NULL);

    lwqq_async_dispatch(lc,FRIEND_COMPLETE,ac);
    //qq_set_basic_info(0,ac);


    //lwqq_info_get_all_friend_qqnumbers(lc,&err);
    /*lock = lwqq_async_evset_new();
    LwqqBuddy* buddy;
    LIST_FOREACH(buddy,&ac->qq->friends,entries){
        event = lwqq_info_get_friend_qqnumber(lc,buddy);
        //lwqq_async_add_event_listener(event,friend_come,buddy);
        lwqq_async_evset_add_event(lock,event);
    }
    LwqqGroup* group;
    LIST_FOREACH(group,&ac->qq->groups,entries) {
        event = lwqq_info_get_group_qqnumber(lc,group);
        lwqq_async_evset_add_event(lock,event);
    }
    lwqq_async_add_evset_listener(lock,qq_set_basic_info,ac);*/

    return NULL;
}
void background_friends_info(qq_account* ac)
{
    START_THREAD(_background_friends_info,ac);
}
void background_msg_poll(qq_account* ac)
{
    LwqqRecvMsgList *l = (LwqqRecvMsgList *)ac->qq->msg_list;

    /* Poll to receive message */
    l->poll_msg(l);
}
void background_msg_drain(qq_account* ac)
{
    LwqqRecvMsgList *l = (LwqqRecvMsgList *)ac->qq->msg_list;
    pthread_cancel(l->tid);
    l->tid = 0;
    /*purple_timeout_remove(msg_check_handle);
    tid = 0;*/
}

static void* _background_upload_file(void* d)
{
    void **data = d;
    LwqqClient* lc = data[0];
    LwqqMsgOffFile* file = data[1];
    LWQQ_PROGRESS progress = data[2];
    PurpleXfer* xfer = data[3];
    s_free(d);
    lwqq_msg_upload_file(lc,file,progress,xfer);
    printf("quit upload file");
    return NULL;
}

void background_upload_file(LwqqClient* lc,LwqqMsgOffFile* file,LWQQ_PROGRESS file_trans_on_progress,PurpleXfer* xfer)
{
    void** data = s_malloc(sizeof(void*)*4);
    data[0] = lc;
    data[1] = file;
    data[2] = file_trans_on_progress;
    data[3] = xfer;
    START_THREAD(_background_upload_file,data);
}
