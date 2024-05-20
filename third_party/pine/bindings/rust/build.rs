use std::path::Path;
use std::env;

fn main() {
    let dir = env::current_dir().unwrap();
    println!("cargo:rustc-link-search=native={}", Path::new(&dir).display());
}
