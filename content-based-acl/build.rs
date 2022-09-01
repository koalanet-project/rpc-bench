const PROTO: &str = "../src/protos/lat_tput_app.proto";
fn main() -> Result<(), Box<dyn std::error::Error>> {
    println!("cargo:rerun-if-changed={PROTO}");
    prost_build::compile_protos(&[PROTO], &["../src/protos"])?;
    Ok(())
}
