fn main() {
    println!("cargo:rerun-if-changed=./proto/rpc_hello/rpc_hello.proto");
    println!("cargo:rerun-if-changed=volo.yml");
    volo_build::ConfigBuilder::default().write().unwrap();
}
