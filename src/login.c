/*
 * Copyright (c) 2021 Benoist André
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <kore/kore.h>
#include <kore/http.h>
#include <kore/curl.h>

#include <unistd.h>

#include "steamauth.h"
#include "assets.h"

int serve_steam_login(struct http_request* req);

static int steam_login_setup(struct http_request* req);
static int steam_login_result(struct http_request* req);

static struct http_state states[] =
{
	KORE_HTTP_STATE(steam_login_setup),
	KORE_HTTP_STATE(steam_login_result)
};

int serve_steam_login(struct http_request* req)
{
	// Steam logins can only performed with GET
	if (req->method != HTTP_METHOD_GET)
	{
		http_response(req, HTTP_STATUS_BAD_REQUEST, NULL, 0);
		return (KORE_RESULT_OK);
	}
	return (http_state_run(states, 2, req));
}

static int steam_login_setup(struct http_request* req)
{
	// GET our parameters
	http_populate_qs(req);
	
	char* claimed_id = NULL;
	if (http_argument_get_string(req, "openid.claimed_id", &claimed_id) == (KORE_RESULT_OK) && claimed_id)
	{
		// Validate the openID with STEAM
		int error;
		if ((error = steam_validate_auth(req)) > 0)
		{
			http_response(req, (error == 1) ? HTTP_STATUS_UNAUTHORIZED : HTTP_STATUS_INTERNAL_ERROR, NULL, 0);
			return (HTTP_STATE_COMPLETE);
		}
		// Sleep & wait for STEAM response
		req->fsm_state++;
		return (HTTP_STATE_RETRY);
	}
	
	// No open id GET parameters, display our login button then
	http_response(req, HTTP_STATUS_OK, asset_login_form_html, asset_len_login_form_html);
	http_response_header(req, "content-type", "text/html; charset=utf-8");
	return (HTTP_STATE_COMPLETE);
}

static int steam_login_result(struct http_request *req)
{
	struct steam_auth* login = http_state_get(req);
	
	// CURL request failed
	if (!kore_curl_success(&login->curl))
	{
		kore_curl_logerror(&login->curl);
		http_response(req, HTTP_STATUS_INTERNAL_ERROR, NULL, 0);
	}
	else
	{
		// For whatever reasons http response code is always 0
		/* long int http_code;
		curl_easy_getinfo(login->curl.handle, CURLINFO_RESPONSE_CODE, &http_code); */
		
		// Got a response back strcmp it
		const char *response = kore_curl_response_as_string(&login->curl);
		
		if (response && strcmp(response, "ns:http://specs.openid.net/auth/2.0\nis_valid:true\n") == 0)
		{
			// SteamID validated
			http_response(req, HTTP_STATUS_OK, NULL, 0);
		}
		else
		{
			// SteamID couldn't be validated
			http_response(req, HTTP_STATUS_UNAUTHORIZED, NULL, 0);
		}
	}
	
	kore_curl_cleanup(&login->curl);
	return (HTTP_STATE_COMPLETE);
}