#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/timerfd.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>

#include "config.h"
#include "uplink.h"

poll_t poll_fds;
CURLM* multi_handle;
int    still_running = 0;

static int poll_fdn = -1;

static inline int events_poll()
{
    return poll(poll_fds.fds, poll_fds.n, POLL_TIMEOUT);
}

int uplink_events_poll()
{
    int rc = 0;
    CURLMsg *m;
    CURLMcode mc = curl_multi_perform(multi_handle, &still_running);

    mc = curl_multi_poll(multi_handle, NULL, 0, 1000, &rc);

    do 
    {
      int queued = 0;
      m = curl_multi_info_read(multi_handle, &queued);
      if(m) 
      {
        if(m && (m->msg == CURLMSG_DONE)) 
        {
            int*  status = NULL;
            char* url = NULL;
            CURL *e = m->easy_handle;
            curl_easy_getinfo(e, CURLINFO_EFFECTIVE_URL, &url);
            curl_easy_getinfo(e, CURLINFO_PRIVATE, &status);
            DBG("Transfer completed '%s' => %d", url ? url : "NULL", m->data.result);
            if(status) *status = m->data.result; 
            curl_multi_remove_handle(multi_handle, e);
            curl_easy_cleanup(e);
        }
      }
    } while(m);

    return events_poll();
}

void init_uplink()
{
    memset(&poll_fds, sizeof(poll_fds), 0);
    multi_handle = curl_multi_init();
}

void setup_uplink_poll()
{
    int n = poll_fds.n++;
    poll_fds.fds[n].fd = -1;
    poll_fds.fds[n].events = 0;
    poll_fds.fds[n].revents = 0;
    poll_fdn = n;
}

void uplink_send_stats(int n, int gr, int ar, int ht, int vt, int cr, int wt)
{
    static int status[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    char  url[1024];
    CURL* hd;

    if(status[n] < 0)
    {
        DBG("Error: Stats %d busy!", n);
        return;
    }

    if(status[n] > 0)
    {
        DBG("Error: Request %d error: %d", n, status[n]);
// TODO: Count errors !!!        
    }

    status[n] = -1;
    sprintf(url, "%s/stats.php??n=%d&gr=%d&ar=%d&ht=%d&vt=%d&cr=%d&wt=%d", cfg.url, n, gr, ar, ht, vt, cr, wt);
    DBG("Send: %s", url);

    hd = curl_easy_init();
    curl_easy_setopt(hd, CURLOPT_URL, url);
    curl_easy_setopt(hd, CURLOPT_PRIVATE, &status[n]);

    curl_multi_add_handle(multi_handle, hd);
}

