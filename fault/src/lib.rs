use proxy_wasm::traits::{Context, HttpContext};
use proxy_wasm::types::{Action, LogLevel};

use getrandom::getrandom;
use prost::Message;
// pub mod lat_tput_app {
//     include!(concat!(env!("OUT_DIR"), "/rpc_bench.lat_tput_app.rs"));
// }

fn generate_random_number(min: i32, max: i32) -> Result<i32, getrandom::Error> {
    let range = max - min;
    let mut buffer = [0u8; 4]; // 4 bytes for a 32-bit integer

    getrandom(&mut buffer)?;

    let random_bytes = u32::from_ne_bytes(buffer) as i32;
    let random_number = min + (random_bytes % (range + 1));

    Ok(random_number)
}

pub mod reservation {
    include!(concat!(env!("OUT_DIR"), "/reservation.rs"));
}

#[no_mangle]
pub fn _start() {
    proxy_wasm::set_log_level(LogLevel::Trace);
    proxy_wasm::set_http_context(|_, _| -> Box<dyn HttpContext> {
        Box::new(Fault { probility: 20 })
    });
}

struct Fault {
    #[allow(unused)]
    probility: u32,
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
        if generate_random_number(0, 100).unwrap() < self.probility as i32 {
            Action::Pause
        } else {
            Action::Continue
        }
    }
}
