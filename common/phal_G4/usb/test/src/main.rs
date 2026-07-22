use std::{env, error::Error, process, time::Duration};

use rusb::{Context, DeviceHandle, UsbContext};

const USB_VENDOR_ID: u16 = 0x1209;
const USB_PRODUCT_ID: u16 = 0x0003;
const USB_INTERFACE: u8 = 0;
const USB_ENDPOINT_OUT: u8 = 0x01;
const USB_ENDPOINT_IN: u8 = 0x81;
const USB_FULL_SPEED_MAX_PACKET_BYTES: usize = 64;
const DEFAULT_ITERATIONS: u32 = 100;
const DEFAULT_PACKET_LENGTH: usize = USB_FULL_SPEED_MAX_PACKET_BYTES;
const DEFAULT_TIMEOUT_MS: u64 = 1_000;

struct Options {
    iterations: u32,
    packet_length: usize,
    timeout: Duration,
}

fn print_usage(program: &str) {
    println!(
        "Usage: {program} [iterations] [packet-length] [timeout-ms]\n\n\
         Defaults: {DEFAULT_ITERATIONS} iterations, {DEFAULT_PACKET_LENGTH}-byte packets, \
         {DEFAULT_TIMEOUT_MS} ms timeout\n\
         The device must be running source/g4_testing with G4_TESTING_CHOSEN=TEST_USB."
    );
}

fn parse_options() -> Result<Options, Box<dyn Error>> {
    let arguments: Vec<String> = env::args().collect();
    if arguments
        .iter()
        .any(|argument| argument == "-h" || argument == "--help")
    {
        print_usage(&arguments[0]);
        process::exit(0);
    }

    if arguments.len() > 4 {
        print_usage(&arguments[0]);
        return Err("too many arguments".into());
    }

    let iterations = arguments
        .get(1)
        .map_or(Ok(DEFAULT_ITERATIONS), |value| value.parse::<u32>())?;
    let packet_length = arguments
        .get(2)
        .map_or(Ok(DEFAULT_PACKET_LENGTH), |value| value.parse::<usize>())?;
    let timeout_ms = arguments
        .get(3)
        .map_or(Ok(DEFAULT_TIMEOUT_MS), |value| value.parse::<u64>())?;

    if iterations == 0 {
        return Err("iterations must be greater than zero".into());
    }
    if packet_length == 0 || packet_length > USB_FULL_SPEED_MAX_PACKET_BYTES {
        return Err(format!(
            "packet-length must be between 1 and {USB_FULL_SPEED_MAX_PACKET_BYTES}"
        )
        .into());
    }
    if timeout_ms == 0 {
        return Err("timeout-ms must be greater than zero".into());
    }

    Ok(Options {
        iterations,
        packet_length,
        timeout: Duration::from_millis(timeout_ms),
    })
}

fn open_g4_usb_device(context: &Context) -> Result<DeviceHandle<Context>, Box<dyn Error>> {
    for device in context.devices()?.iter() {
        let descriptor = device.device_descriptor()?;
        if descriptor.vendor_id() != USB_VENDOR_ID || descriptor.product_id() != USB_PRODUCT_ID {
            continue;
        }

        let handle = device.open()?;
        handle.set_auto_detach_kernel_driver(true).ok();
        handle.set_active_configuration(1)?;
        handle.claim_interface(USB_INTERFACE)?;
        println!(
            "Connected to STM32G4 USB test device on bus {} address {}",
            device.bus_number(),
            device.address()
        );
        return Ok(handle);
    }

    Err(format!(
        "STM32G4 USB test device {:04x}:{:04x} was not found",
        USB_VENDOR_ID, USB_PRODUCT_ID
    )
    .into())
}

fn packet_for_iteration(iteration: u32, packet_length: usize) -> Vec<u8> {
    (0..packet_length)
        .map(|index| {
            let value = iteration.wrapping_mul(31).wrapping_add(index as u32);
            value as u8
        })
        .collect()
}

fn run_echo_test(
    handle: &mut DeviceHandle<Context>,
    options: &Options,
) -> Result<(), Box<dyn Error>> {
    for iteration in 0..options.iterations {
        let packet = packet_for_iteration(iteration, options.packet_length);
        let written = handle.write_bulk(USB_ENDPOINT_OUT, &packet, options.timeout)?;
        if written != packet.len() {
            return Err(format!("short USB write: {written}/{} bytes", packet.len()).into());
        }

        let mut echoed = [0u8; USB_FULL_SPEED_MAX_PACKET_BYTES];
        let read = handle.read_bulk(USB_ENDPOINT_IN, &mut echoed, options.timeout)?;
        if read != packet.len() {
            return Err(format!("short USB read: {read}/{} bytes", packet.len()).into());
        }
        if echoed[..read] != packet[..] {
            return Err(format!("echo mismatch on iteration {iteration}").into());
        }

        println!(
            "iteration {:>4}/{}: echoed {} bytes",
            iteration + 1,
            options.iterations,
            read
        );
    }

    Ok(())
}

fn main() -> Result<(), Box<dyn Error>> {
    let options = parse_options()?;
    let context = Context::new()?;
    let mut handle = open_g4_usb_device(&context)?;

    if let Err(error) = run_echo_test(&mut handle, &options) {
        eprintln!("USB echo test failed: {error}");
        process::exit(1);
    }

    println!("USB echo test passed");
    Ok(())
}
