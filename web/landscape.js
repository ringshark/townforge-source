(function () {
  'use strict';
  function fit(width, height, touch) {
    const rotated = touch && height > width;
    const w = rotated ? height : width, h = rotated ? width : height;
    const tablet = touch && Math.min(w, h) >= 600;
    const fillPhone = touch && !tablet;
    const canvasWidth = fillPhone ? w : Math.min(w, h * 16 / 9);
    return { rotated, width: w, height: h, canvasWidth, canvasHeight: fillPhone ? h : canvasWidth * 9 / 16 };
  }
  function point(x, y, rect, rotated) {
    const u = (x - rect.left) / rect.width, v = (y - rect.top) / rect.height;
    return rotated ? { x: v * 960, y: (1 - u) * 540 } : { x: u * 960, y: v * 540 };
  }
  function coordinates(t, rect, scrollX, scrollY) {
    const p = point(t.clientX, t.clientY, rect, true);
    const clientX = rect.left + p.x / 960 * rect.width;
    const clientY = rect.top + p.y / 540 * rect.height;
    return { clientX, clientY, pageX: clientX + scrollX, pageY: clientY + scrollY };
  }
  if (typeof module !== 'undefined') module.exports = { fit, point, coordinates };
  if (typeof document === 'undefined') return;
  const canvas = document.getElementById('canvas'), stage = canvas.parentElement;
  const touch = navigator.maxTouchPoints > 0 || matchMedia('(pointer:coarse)').matches;
  let rotated = false, dragging = false;
  const synthetic = new WeakSet();
  function layout() {
    const f = fit(innerWidth, innerHeight, touch);
    rotated = f.rotated;
    stage.style.position = 'fixed'; stage.style.left = '50%'; stage.style.top = '50%';
    stage.style.width = f.width + 'px'; stage.style.height = f.height + 'px';
    stage.style.transform = 'translate(-50%, -50%)' + (rotated ? ' rotate(90deg)' : '');
    // The world fills the phone, including the area around the system insets.
    stage.style.padding = touch ? '0' : '';
    canvas.style.width = f.canvasWidth + 'px'; canvas.style.height = f.canvasHeight + 'px';
  }
  function lock() {
    try {
      if (touch && screen.orientation && screen.orientation.lock) screen.orientation.lock('landscape').catch(function () {});
    } catch (_) {}
  }
  function remap(event) {
    if (!rotated || synthetic.has(event)) return;
    const start = event.type === 'mousedown' || event.type === 'touchstart';
    if (start) dragging = event.target === canvas;
    if (event.target !== canvas && !dragging) return;
    const rect = canvas.getBoundingClientRect();
    let replacement;
    if (event.type.startsWith('touch')) {
      replacement = new Event(event.type, { bubbles: true, cancelable: true });
      for (const key of ['touches', 'changedTouches', 'targetTouches']) {
        const list = Array.from(event[key] || [], function (t) {
          // GLFW reads page coordinates for touches, while raylib also reads
          // client coordinates. Keep both in the inverse-rotated space.
          return Object.assign({ identifier: t.identifier, target: t.target, screenX: t.screenX, screenY: t.screenY }, coordinates(t, rect, window.scrollX || 0, window.scrollY || 0));
        });
        Object.defineProperty(replacement, key, { value: list });
      }
    } else {
      replacement = new MouseEvent(event.type, Object.assign({ bubbles: true, cancelable: true, button: event.button, buttons: event.buttons }, coordinates(event, rect, window.scrollX || 0, window.scrollY || 0)));
    }
    synthetic.add(replacement);
    event.preventDefault(); event.stopImmediatePropagation();
    event.target.dispatchEvent(replacement);
    if (event.type === 'mouseup' || (event.type === 'touchend' && !event.touches.length) || event.type === 'touchcancel') dragging = false;
  }
  for (const type of ['mousedown', 'mousemove', 'mouseup', 'touchstart', 'touchmove', 'touchend', 'touchcancel']) window.addEventListener(type, remap, { capture: true, passive: false });
  addEventListener('resize', layout); addEventListener('orientationchange', layout);
  addEventListener('fullscreenchange', layout);
  if (window.visualViewport) window.visualViewport.addEventListener('resize', layout);
  addEventListener('pointerup', function () {
    if (!touch) return;
    if (document.documentElement.requestFullscreen && !document.fullscreenElement) document.documentElement.requestFullscreen().then(lock).catch(lock);
    else lock();
  }, { once: true });
  layout(); lock();
})();
