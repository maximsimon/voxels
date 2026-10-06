#!/usr/bin/env python3
"""
WrapperWindow: single mosaic window for VecEnv runs.
Toggle: h=hide, s=show. Hidden mode skips waitKey except every POLL_EVERY calls.
"""
import numpy as np
import cv2

POLL_EVERY = 100

# one mosaic window showing all N camera feeds in a resizable, wrapping grid.
class WrapperWindow:
	def __init__(self, n_envs):
		self._shown = True
		self._step = 0
		self._mosaic_name = "VecEnv wrapper"
		self._ctrl_name = "VecEnv ctrl"
		self._ctrl = np.zeros((40, 280, 3), np.uint8)
		self._ctrl[:] = (30, 30, 30)
		cv2.putText(self._ctrl, "H: hide  S: show", (10, 28), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (180,180,180), 1)
		cv2.namedWindow(self._ctrl_name)
		cv2.imshow(self._ctrl_name, self._ctrl)
		cv2.waitKey(1)
		
	# show mosaic if visible; throttle key poll when hidden.
	def update(self, frames):
		if frames is None:
			return
		self._step += 1
		if self._shown:
			self._draw_mosaic(frames)
		elif self._step % POLL_EVERY == 0:
			cv2.imshow(self._ctrl_name, self._ctrl)
			k = cv2.waitKey(1) & 0xFF
			if k == ord('s'):
				self._shown = True

	def _draw_mosaic(self, frames):
		N = frames.shape[0]
		h, w = frames.shape[1], frames.shape[2]
		frame_aspect = w / h

		# recreate the window whenever it is gone (incl. after hide/show); the
		# WINDOW_NORMAL flag stops imshow from auto-shrinking it to the canvas.
		if cv2.getWindowProperty(self._mosaic_name, cv2.WND_PROP_VISIBLE) < 1:
			cv2.namedWindow(self._mosaic_name, cv2.WINDOW_NORMAL)
			cv2.resizeWindow(self._mosaic_name, 1280, 720)

		# read the live window size so the grid reflows when the user resizes
		rect = cv2.getWindowImageRect(self._mosaic_name)
		sw = float(rect[2]) if rect and rect[2] > 0 else 1280.0
		sh = float(rect[3]) if rect and rect[3] > 0 else 720.0
		margin = 8.0
		avail_w, avail_h = sw - 2 * margin, sh - 2 * margin

		# choose the column count whose largest aspect-preserving pane still fits
		best = (1, -1.0)
		for cols in range(1, N + 1):
			rows = (N + cols - 1) // cols
			pane_w = avail_w / cols
			pane_h = min(pane_w / frame_aspect, avail_h / rows)
			if pane_h > best[1]:
				best = (cols, pane_h)
		cols, pane_h = best
		rows = (N + cols - 1) // cols
		pane_w = pane_h * frame_aspect
		pw, ph = max(1, round(pane_w)), max(1, round(pane_h))

		canvas = np.zeros((rows * ph, cols * pw, 3), np.uint8)
		for i in range(N):
			r, c = divmod(i, cols)
			p = cv2.resize(frames[i], (pw, ph))
			p = cv2.putText(p, str(i), (4, 24),
							cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0,255,0), 1)
			canvas[r*ph:(r+1)*ph, c*pw:(c+1)*pw] = p

		cv2.imshow(self._mosaic_name, canvas)
		k = cv2.waitKey(1) & 0xFF
		if k == ord('h'):
			cv2.destroyWindow(self._mosaic_name)
			self._shown = False

	def close(self):
		cv2.destroyAllWindows()
