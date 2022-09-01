use log::info;
use proxy_wasm::traits::*;
use proxy_wasm::types::*;

#[no_mangle]
pub fn _start() {
    proxy_wasm::set_log_level(LogLevel::Trace);
    proxy_wasm::set_http_context(|context_id, _| -> Box<dyn HttpContext> {
        Box::new(AccessControl { context_id })
    });
}

struct AccessControl {
    context_id: u32,
}

impl Context for AccessControl {}

impl HttpContext for AccessControl {
    fn on_http_request_headers(&mut self, num_of_headers: usize, end_of_stream: bool) -> Action {
        info!("on_http_request_headers, num_of_headers: {num_of_headers}, end_of_stream: {end_of_stream}");
        if end_of_stream {
            for (name, value) in &self.get_http_request_headers() {
                info!("In WASM : #{} -> {}: {}", self.context_id, name, value);
            }
        }

        Action::Continue
        // match self.get_http_request_header("token") {
        //     Some(token) if token.parse::<u64>().is_ok() && is_prime(token.parse().unwrap()) => {
        //         self.resume_http_request();
        //         Action::Continue
        //     }
        //     _ => {
        //         self.send_http_response(
        //             403,
        //             vec![("Powered-By", "proxy-wasm")],
        //             Some(b"Access forbidden.\n"),
        //         );
        //         Action::Pause
        //     }
        // }
    }

    fn on_http_request_body(&mut self, body_size: usize, end_of_stream: bool) -> Action {
        info!("on_http_request_body, body_size: {body_size}, end_of_stream: {end_of_stream}");
        if end_of_stream {
            info!("body_size: {}", body_size);
        }
        if let Some(body) = self.get_http_request_body(0, body_size) {
            // Parse grpc payload
            info!("body: {:?}", body);
        }
        Action::Continue
    }
}
