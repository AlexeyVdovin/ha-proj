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
    return poll(poll_fds.fds, poll_fds.n, 1);
}

int uplink_events_poll()
{
    int rc = 0;
    CURLMsg *m;
    CURLMcode mc = curl_multi_perform(multi_handle, &still_running);

    mc = curl_multi_poll(multi_handle, NULL, 0, POLL_TIMEOUT, &rc);

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
            if(m->data.result != 0) DBG("Transfer completed '%s' => %d", url ? url : "NULL", m->data.result);
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
    static int errors[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    static int timeout[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    static CURL* hds[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    char  url[1024];
    CURL* hd;

    if(n < 0 || n > sizeof(status)/sizeof(status[0]))
    {
        DBG("Error: Incorrect %d request!", n);
        return;
    }
    if(status[n] < 0)
    {
        if(timeout[n] > time(0))
        {
            DBG("Error: Stats %d busy!", n);
            return;
        }
        hd = hds[n];
        if(hd)
        {
            curl_multi_remove_handle(multi_handle, hd);
            curl_easy_cleanup(hd);
            hds[n] = NULL;
        }
    }
    if(status[n] > 0)
    {
        errors[n]++;
        DBG("Error: Request %d error: %d:%d", n, status[n], errors[n]);
    }
    else
    {
        errors[n] = 0;
    }

    if(n == 1 || n == 2)
    { // Green stats
        status[n] = -1;
        sprintf(url, "%s/stats.php?n=%d&gr=%d&ar=%d&ht=%d&vt=%d&cr=%d&wt=%d", cfg.url, n, gr, ar, ht, vt, cr, wt);
    }
    else if(n == 0)
    { // BMC telemetry
        status[n] = -1;
        sprintf(url, "%s/bmc.php?ad12v=%d&batt=%d&z5v0=%d&z3v3=%d", cfg.url, gr, ar, ht, vt);
    }
    else
    {
        DBG("Error: Not implemented %d request!", n);
        return;
    }
    DBG("Send: %s", url);

    hd = curl_easy_init();
    curl_easy_setopt(hd, CURLOPT_URL, url);
    curl_easy_setopt(hd, CURLOPT_PRIVATE, &status[n]);

    curl_multi_add_handle(multi_handle, hd);
    hds[n] = hd;
    timeout[n] = time(0) + 300; // 5 min
}

