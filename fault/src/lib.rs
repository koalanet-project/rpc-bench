use proxy_wasm::traits::{Context, HttpContext};
use proxy_wasm::types::{Action, LogLevel};

use prost::Message;
use rand::Rng;
// pub mod lat_tput_app {
//     include!(concat!(env!("OUT_DIR"), "/rpc_bench.lat_tput_app.rs"));
// }

pub mod reservation {
    include!(concat!(env!("OUT_DIR"), "/reservation.rs"));
}

#[no_mangle]
pub fn _start() {
    proxy_wasm::set_log_level(LogLevel::Trace);
    proxy_wasm::set_http_context(|probility, _| -> Box<dyn HttpContext> {
        Box::new(Fault {
            probility: (probility as f32) / 100.0,
        })
    });
}

struct Fault {
    #[allow(unused)]
    probility: f32,
}

impl Context for Fault {}

impl HttpContext for Fault {
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

        // if rand number > 0.2
        if rand::random::<f32>() < self.probility {
            Action::Pause
        } else {
            Action::Continue
        }
    }
}
