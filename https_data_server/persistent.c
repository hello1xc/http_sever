#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include <openssl/ssl.h>
#include <openssl/err.h>


#include <cJSON.h>
#include "util.h"

void persistent_cb (struct evhttp_request *req, void *arg)
{ 
    struct evbuffer *evb = NULL;
    const char *uri = evhttp_request_get_uri (req);
    struct evhttp_uri *decoded = NULL;

    /* 判断 req 是否是GET 请求 */
    if (evhttp_request_get_command (req) == EVHTTP_REQ_GET)
    {
        struct evbuffer *buf = evbuffer_new();
        if (buf == NULL) return;
        evbuffer_add_printf(buf, "Requested: %s\n", uri);
        evhttp_send_reply(req, HTTP_OK, "OK", buf);
        return;
    }

    /* 这里只处理Post请求, Get请求，就直接return 200 OK  */
    if (evhttp_request_get_command (req) != EVHTTP_REQ_POST)
    { 
        evhttp_send_reply (req, 200, "OK", NULL);
        return;
    }

    printf ("Got a POST request for <%s>\n", uri);

    //判断此URI是否合法
    decoded = evhttp_uri_parse (uri);
    if (! decoded)
    { 
        printf ("It's not a good URI. Sending BADREQUEST\n");
        evhttp_send_error (req, HTTP_BADREQUEST, 0);
        return;
    }

    /* Decode the payload */
    struct evbuffer *buf = evhttp_request_get_input_buffer (req);
    evbuffer_add (buf, "", 1);    /* NUL-terminate the buffer */
    char *payload = (char *) evbuffer_pullup (buf, -1);
    int post_data_len = evbuffer_get_length(buf);
    char request_data_buf[4096] = {0};
    memcpy(request_data_buf, payload, post_data_len);

    printf("[post_data][%d]=\n %s\n", post_data_len, payload);


    /*
       具体的：可以根据Post的参数执行相应操作，然后将结果输出
       ...
    */
    /*
     *
     *    https://ip:port/persistent [json_data]  
     *        {
     *            cmd: "insert",
     *            busi: "reg",
     *            table: "OBO_TABLE_USER",

     *            username:  "盖伦",
     *            password:  "ADSWADSADWQ(MD5加密之后的)",
     *            tel     :  "13332133313",
     *            email   :  "danbing_at@163.com",
     *            id_card :  "21040418331323",
     *            driver  :  "yes",
     *         }
     *
     *
     *
     *
     * */

    int is_succ = 0; //0 succ, -1 fail;
    cJSON *root= cJSON_Parse(request_data_buf);
    char *reason = NULL;

    //unpack json

    cJSON *cmd = cJSON_GetObjectItem(root, "cmd");
    if (strcmp(cmd->valuestring, "insert") == 0) {
        //插入业务

        cJSON* table = cJSON_GetObjectItem(root, "table");
        if (strcmp(table->valuestring, "OBO_TABLE_USER")== 0) {
            //入库用户表的业务
            cJSON *username = cJSON_GetObjectItem(root, "username");
            cJSON *password = cJSON_GetObjectItem(root, "password");
            cJSON *driver = cJSON_GetObjectItem(root, "driver");
            cJSON *email = cJSON_GetObjectItem(root, "email");
            cJSON *tel = cJSON_GetObjectItem(root, "tel");
            cJSON *id_card = cJSON_GetObjectItem(root, "id_card");

            printf("username = %s\n", username->valuestring);
            printf("password = %s\n", password->valuestring);
            printf("email = %s\n", email->valuestring);
            printf("tel = %s\n", tel->valuestring);
            printf("driver = %s\n", driver->valuestring);
            printf("id_card = %s\n", id_card->valuestring);

            printf("insert %s \n" ,table->valuestring);
            //mysql的入库接口
            //
            //得到是否入库成功


            is_succ = 0;
            reason = "insert table error";
        }
    }
    else if (strcmp(cmd->valuestring, "query") == 0) {
        //查询的业务
    }





    


    char *response_data  = NULL;

    cJSON *response_web = cJSON_CreateObject();
    if (is_succ == 0) {
        cJSON_AddStringToObject(response_web, "result", "ok");
    }
    else {
        cJSON_AddStringToObject(response_web, "result", "error");
        cJSON_AddStringToObject(response_web, "reason", reason);
    }

    response_data = cJSON_Print(response_web);

    /* This holds the content we're sending. */

    //HTTP header
    evhttp_add_header(evhttp_request_get_output_headers(req), "Server", MYHTTPD_SIGNATURE);
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/plain; charset=UTF-8");
    evhttp_add_header(evhttp_request_get_output_headers(req), "Connection", "close");

    evb = evbuffer_new ();
    evbuffer_add_printf(evb, "%s", response_data);
    //将封装好的evbuffer 发送给客户端
    evhttp_send_reply(req, HTTP_OK, "OK", evb);

    if (decoded)
        evhttp_uri_free (decoded);
    if (evb)
        evbuffer_free (evb);


    printf("[response]:\n");
    printf("%s\n", response_data);

    free(response_data);
    cJSON_Delete(response_web);
    cJSON_Delete(root);
}
