```
pushd ../content-based-acl
cargo build --target=wasm32-unknown-unknown --release
cp target/wasm32-unknown-unknown/release/content-based-acl.wasm /tmp
popd
sudo docker compose up --build
```
