#![no_std]

use core::{cell::UnsafeCell, panic::PanicInfo};
use mejiro_engine::{
    Error, Mode, Session, State, Translation, ffi::CEvent, ffi::CTranslation, translate,
};

// QMK calls this from its single keyboard event loop. Never call it from an ISR.
struct Engine(UnsafeCell<Session<20>>);
unsafe impl Sync for Engine {}
static ENGINE: Engine = Engine(UnsafeCell::new(Session::new()));
// Copy a flash-resident template directly into the engine. Constructing a
// Session on the stack also needs a second large temporary inside Session::new;
// together those frames exceed the RP2040's 32 KiB process stack at boot.
static INITIAL_SESSION: Session<20> = Session::new();

#[panic_handler]
fn panic(_: &PanicInfo<'_>) -> ! {
    loop {
        core::hint::spin_loop();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn mejiro_qmk_init() {
    unsafe { core::ptr::copy_nonoverlapping(&INITIAL_SESSION, ENGINE.0.get(), 1) };
    let session = unsafe { &mut *ENGINE.0.get() };
    session.set_typing_mode(Mode::Romaji).unwrap();
    session.process("#-n").unwrap();
}

#[unsafe(no_mangle)]
pub extern "C" fn mejiro_qmk_reset() {
    // Input context boundaries preserve recording; stopping is explicit.
    unsafe { &mut *ENGINE.0.get() }.reset();
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mejiro_qmk_record_external(
    events: *const mejiro_engine::HidEvent,
    count: usize,
) -> i32 {
    unsafe {
        mejiro_engine::ffi::mejiro_core_session_record_external(
            ENGINE.0.get().cast(),
            events,
            count,
        )
    }
}
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mejiro_qmk_replayed_external(
    output: *mut mejiro_engine::RecordedEvent,
    capacity: usize,
) -> i32 {
    unsafe {
        mejiro_engine::ffi::mejiro_core_session_replayed_external(
            ENGINE.0.get().cast(),
            output,
            capacity,
        )
    }
}
#[unsafe(no_mangle)]
pub extern "C" fn mejiro_qmk_abort_recordings() {
    unsafe { &mut *ENGINE.0.get() }.abort_recordings();
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mejiro_qmk_process(
    stroke: *const u8,
    len: usize,
    event: *mut CEvent,
) -> i32 {
    unsafe {
        mejiro_engine::ffi::mejiro_core_session_process(ENGINE.0.get().cast(), stroke, len, event)
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn mejiro_qmk_ack(ticket: u64, units: u32) -> i32 {
    unsafe { &mut *ENGINE.0.get() }
        .acknowledge(ticket, units)
        .map_or(-1, |_| 0)
}

// A kana preview supplies the host's deletion metric without changing the
// committed Romaji session. It is only a hint until the adapter emits keys.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mejiro_qmk_preview_kana(
    stroke: *const u8,
    len: usize,
    output: *mut CTranslation,
) -> i32 {
    if stroke.is_null() || output.is_null() {
        return -1;
    }
    let bytes = unsafe { core::slice::from_raw_parts(stroke, len) };
    let raw = match core::str::from_utf8(bytes) {
        Ok(raw) => raw,
        Err(_) => return -1,
    };
    let mut state: State = unsafe { &*ENGINE.0.get() }.state();
    state.mode = Mode::Kana;
    let result: Translation = match translate(raw, state, true) {
        Ok(value) => value,
        Err(Error::Capacity) => return -2,
        Err(_) => return -1,
    };
    unsafe { output.write(result.into()) };
    0
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mejiro_qmk_preview(
    stroke: *const u8,
    len: usize,
    output: *mut CTranslation,
) -> i32 {
    if stroke.is_null() || output.is_null() {
        return -1;
    }
    let bytes = unsafe { core::slice::from_raw_parts(stroke, len) };
    let raw = match core::str::from_utf8(bytes) {
        Ok(raw) => raw,
        Err(_) => return -1,
    };
    let state: State = unsafe { &*ENGINE.0.get() }.state();
    let result: Translation = match translate(raw, state, true) {
        Ok(value) => value,
        Err(Error::Capacity) => return -2,
        Err(_) => return -1,
    };
    unsafe { output.write(result.into()) };
    0
}

#[unsafe(no_mangle)]
pub extern "C" fn mejiro_qmk_is_recording(index: usize) -> bool {
    unsafe { &*ENGINE.0.get() }
        .recording(index)
        .unwrap_or(false)
}
