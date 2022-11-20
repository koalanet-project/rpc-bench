const PROTO: &str = "./proto/rpc_hello/rpc_hello.proto";
const PROTO_DIR: &str = "proto";
const PROTOS: &[&str] = &[PROTO];

fn main() -> Result<(), Box<dyn std::error::Error>> {
    println!("cargo:rerun-if-changed={PROTO}");
    let mut config = prost_build::Config::new();
    config.bytes(&["."]);
    tonic_build::configure().compile_with_config(config, PROTOS, &[PROTO_DIR])?;
    Ok(())
}
