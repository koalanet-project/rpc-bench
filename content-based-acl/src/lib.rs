use proxy_wasm::traits::{Context, HttpContext};
use proxy_wasm::types::{Action, LogLevel};

use prost::Message;

// pub mod lat_tput_app {
//     include!(concat!(env!("OUT_DIR"), "/rpc_bench.lat_tput_app.rs"));
// }

pub mod reservation {
    include!(concat!(env!("OUT_DIR"), "/reservation.rs"));
}

#[no_mangle]
pub fn _start() {
    proxy_wasm::set_log_level(LogLevel::Trace);
    proxy_wasm::set_http_context(|context_id, _| -> Box<dyn HttpContext> {
        Box::new(AccessControl { context_id })
    });
}

struct AccessControl {
    #[allow(unused)]
    context_id: u32,
}

impl Context for AccessControl {}

impl HttpContext for AccessControl {
    fn on_http_request_headers(&mut self, _num_of_headers: usize, end_of_stream: bool) -> Action {
        // log::info!("on_http_request_headers, num_of_headers: {num_of_headers}, end_of_stream: {end_of_stream}");

        // for (name, value) in &self.get_http_request_headers() {
        //     log::info!("In WASM : #{} -> {}: {}", self.context_id, name, value);
        // }

        if !end_of_stream {
            return Action::Continue;
        }

        // If there is a Content-Length header and we change the length of
        // the body later, then clients will break. So remove it.
        // We must do this here, because once we exit this function we
        // can no longer modify the response headers.
        self.set_http_response_header("content-length", None);
        Action::Continue
    }

    fn on_http_request_body(&mut self, body_size: usize, end_of_stream: bool) -> Action {
        // log::info!("on_http_request_body, body_size: {body_size}, end_of_stream: {end_of_stream}");
        if !end_of_stream {
            // Wait -- we'll be called again when the complete body is buffered
            // at the host side.
            // Returns Continue because there's other filters to process. Return Pause may cause
            // problem in this case.
            // if let Some(body) = self.get_http_request_body(0, body_size) {
            //     log::info!("body: {:?}", body);
            // }
            // return Action::Continue;
            return Action::Pause;
        }

        // Replace the message body if it contains the text "secret".
        // Since we returned "Pause" previuously, this will return the whole body.
        if let Some(body) = self.get_http_request_body(0, body_size) {

            // log::info!("body: {:?}", body);
            // Parse grpc payload, skip the first 5 bytes
            match reservation::Request::decode(&body[5..]) {
                Ok(req) => {
                    // log::info!("req: {:?}", req);

                    if req.customer_name == "danyang" {
                        // block the user called 'danyang' ;-)
                        self.send_http_response(
                            200,
                            vec![
                                ("grpc-status", "1"),
                                ("grpc-message", "Access forbidden.\n"),
                            ],
                            None,
                        );
                        return Action::Pause;
                    }
                }
                Err(e) => log::warn!("decode error: {}", e),
            }
        }

        Action::Continue
    }
}
