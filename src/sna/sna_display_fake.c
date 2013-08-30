/*
 * Copyright © 2013 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *	Chris Wilson <chris@chris-wilson.co.uk>
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "sna.h"

static void
sna_crtc_dpms(xf86CrtcPtr crtc, int mode)
{
}

static Bool
sna_crtc_set_mode_major(xf86CrtcPtr crtc, DisplayModePtr mode,
			Rotation rotation, int x, int y)
{
	return TRUE;
}

static void
sna_crtc_set_cursor_colors(xf86CrtcPtr crtc, int bg, int fg)
{
}

static void
sna_crtc_set_cursor_position(xf86CrtcPtr crtc, int x, int y)
{
}

static void
sna_crtc_hide_cursor(xf86CrtcPtr crtc)
{
}

static void
sna_crtc_show_cursor(xf86CrtcPtr crtc)
{
}

static void
sna_crtc_load_cursor_argb(xf86CrtcPtr crtc, CARD32 *image)
{
}

static void
sna_crtc_gamma_set(xf86CrtcPtr crtc,
		       CARD16 *red, CARD16 *green, CARD16 *blue, int size)
{
}

static void
sna_crtc_destroy(xf86CrtcPtr crtc)
{
}

#if HAS_PIXMAP_SHARING
static Bool
sna_crtc_set_scanout_pixmap(xf86CrtcPtr crtc, PixmapPtr pixmap)
{
	return TRUE;
}
#endif

static const xf86CrtcFuncsRec sna_crtc_funcs = {
	.dpms = sna_crtc_dpms,
	.set_mode_major = sna_crtc_set_mode_major,
	.set_cursor_colors = sna_crtc_set_cursor_colors,
	.set_cursor_position = sna_crtc_set_cursor_position,
	.show_cursor = sna_crtc_show_cursor,
	.hide_cursor = sna_crtc_hide_cursor,
	.load_cursor_argb = sna_crtc_load_cursor_argb,
	.gamma_set = sna_crtc_gamma_set,
	.destroy = sna_crtc_destroy,
#if HAS_PIXMAP_SHARING
	.set_scanout_pixmap = sna_crtc_set_scanout_pixmap,
#endif
};

static bool
sna_crtc_fake(struct sna *sna)
{
	ScrnInfoPtr scrn = sna->scrn;
	xf86CrtcPtr crtc;

	DBG(("%s\n", __FUNCTION__));

	crtc = xf86CrtcCreate(scrn, &sna_crtc_funcs);
	if (crtc == NULL)
		return false;

	return true;
}

static void
sna_output_create_resources(xf86OutputPtr output)
{
}

static Bool
sna_output_set_property(xf86OutputPtr output, Atom property,
			    RRPropertyValuePtr value)
{
	return TRUE;
}

static Bool
sna_output_get_property(xf86OutputPtr output, Atom property)
{
	return FALSE;
}

static void
sna_output_dpms(xf86OutputPtr output, int dpms)
{
}

static xf86OutputStatus
sna_output_detect(xf86OutputPtr output)
{
	if (output->randr_output->numUserModes)
		return XF86OutputStatusConnected;

	return XF86OutputStatusDisconnected;
}

static Bool
sna_output_mode_valid(xf86OutputPtr output, DisplayModePtr mode)
{
	if (mode->type & M_T_DEFAULT)
		return MODE_BAD;

	return MODE_OK;
}

static DisplayModePtr
sna_output_get_modes(xf86OutputPtr output)
{
	return NULL;
}

static void
sna_output_destroy(xf86OutputPtr output)
{
}

static const xf86OutputFuncsRec sna_output_funcs = {
	.create_resources = sna_output_create_resources,
#ifdef RANDR_12_INTERFACE
	.set_property = sna_output_set_property,
	.get_property = sna_output_get_property,
#endif
	.dpms = sna_output_dpms,
	.detect = sna_output_detect,
	.mode_valid = sna_output_mode_valid,

	.get_modes = sna_output_get_modes,
	.destroy = sna_output_destroy
};

static bool
sna_output_fake(struct sna *sna, int n, int num_fake, int num_real_crtc, int num_real_output)
{
	ScrnInfoPtr scrn = sna->scrn;
	xf86OutputPtr output;
	unsigned mask;
	char buf[80];

	sprintf(buf, "VIRTUAL%d", n+1);
	output = xf86OutputCreate(scrn, &sna_output_funcs, buf);
	if (!output)
		return false;

	output->mm_width = 0;
	output->mm_height = 0;

	output->subpixel_order = SubPixelNone;

	mask = (1 << num_fake) - 1;
	output->possible_crtcs = mask << num_real_crtc;
	output->possible_clones = mask << num_real_output;
	output->interlaceAllowed = FALSE;

	return true;
}

static Bool
sna_mode_resize(ScrnInfoPtr scrn, int width, int height)
{
	ScreenPtr screen = scrn->pScreen;
	PixmapPtr new_front;

	DBG(("%s (%d, %d) -> (%d, %d)\n", __FUNCTION__,
	     scrn->virtualX, scrn->virtualY,
	     width, height));

	if (scrn->virtualX == width && scrn->virtualY == height)
		return TRUE;

	assert(to_sna_from_screen(screen)->front);
	assert(screen->GetScreenPixmap(screen) == to_sna_from_screen(screen)->front);

	DBG(("%s: creating new framebuffer %dx%d\n",
	     __FUNCTION__, width, height));

	new_front = screen->CreatePixmap(screen,
					 width, height, scrn->depth,
					 0);
	if (!new_front)
		return FALSE;

	scrn->virtualX = width;
	scrn->virtualY = height;
	scrn->displayWidth = width;

	screen->SetScreenPixmap(new_front);
	assert(screen->GetScreenPixmap(screen) == new_front);
	assert(to_sna_from_screen(screen)->front == new_front);

	screen->DestroyPixmap(new_front);

	return TRUE;
}

static const xf86CrtcConfigFuncsRec sna_mode_funcs = {
	sna_mode_resize
};

bool sna_mode_fake_init(struct sna *sna, int num_fake)
{
	xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(sna->scrn);
	int n, num_real_crtc, num_real_output;

	if (num_fake == 0)
		return true;

	num_real_crtc = xf86_config->num_crtc;
	num_real_output = xf86_config->num_output;

	if (num_real_crtc == 0)
		xf86CrtcConfigInit(sna->scrn, &sna_mode_funcs);

	for (n = 0; n < num_fake; n++) {
		if (!sna_crtc_fake(sna))
			return false;

		if (!sna_output_fake(sna, n, num_fake,
				     num_real_crtc, num_real_output))
			return false;
	}

	return true;
}
