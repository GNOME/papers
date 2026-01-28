use crate::deps::*;
use image::{GrayImage, Luma, Rgba, RgbaImage};
use imageproc::contrast::{ThresholdType, otsu_level, threshold};

// Minimum alpha value to consider a pixel opaque (0–255 scale).
// Pixels below this are treated as transparent background.
const ALPHA_OPAQUE_THRESHOLD: u8 = 128;

// After Otsu + ToZero thresholding, pixels below this intensity
// are classified as "dark" (potential ink on a light background).
const INK_INTENSITY_THRESHOLD: u8 = 5;

// Raw image data that can be sent across threads
pub struct RawImageData {
    pub data: Vec<u8>,
    pub width: u32,
    pub height: u32,
    pub n_channels: usize,
    pub rowstride: usize,
}

#[allow(dead_code)]
impl RawImageData {
    // Extract raw data from pixbuf (must be called on main thread)
    pub fn from_pixbuf(pixbuf: &gdk::gdk_pixbuf::Pixbuf) -> RawImageData {
        RawImageData {
            data: pixbuf.read_pixel_bytes().to_vec(),
            width: pixbuf.width() as u32,
            height: pixbuf.height() as u32,
            n_channels: pixbuf.n_channels() as usize,
            rowstride: pixbuf.rowstride() as usize,
        }
    }

    // Process image data in background thread - returns RGBA bytes and dimensions
    pub fn do_signature_threshold(&self) -> Result<RawImageData, String> {
        if self.n_channels < 3 {
            return Err("Image must have at least 3 channels (RGB)".to_string());
        }

        let has_alpha = self.n_channels >= 4;

        // Build a grayscale image only from opaque pixels for threshold computation.
        // Transparent pixels are excluded so they don't skew the Otsu threshold.
        let gray_image = GrayImage::from_fn(self.width, self.height, |x, y| {
            let idx = (y as usize * self.rowstride) + (x as usize * self.n_channels);
            let alpha = if has_alpha { self.data[idx + 3] } else { 255 };
            if alpha < ALPHA_OPAQUE_THRESHOLD {
                // Treat transparent pixels as white background for threshold computation
                Luma([255])
            } else {
                let r = self.data[idx] as f32;
                let g = self.data[idx + 1] as f32;
                let b = self.data[idx + 2] as f32;
                let gray = (0.299 * r + 0.587 * g + 0.114 * b) as u8;
                Luma([gray])
            }
        });

        // Compute Otsu threshold and apply
        let threshold_value = otsu_level(&gray_image);
        let thresholded = threshold(&gray_image, threshold_value, ThresholdType::ToZero);

        // After To zero: dark pixels (below threshold), light pixels → kept.
        // Count opaque pixels in each group to determine which is ink vs background.
        // The ink (signature strokes) always covers less area than the background.
        let mut dark_count: u64 = 0;
        let mut light_count: u64 = 0;
        for y in 0..self.height {
            for x in 0..self.width {
                let idx = (y as usize * self.rowstride) + (x as usize * self.n_channels);
                let alpha = if has_alpha { self.data[idx + 3] } else { 255 };
                if alpha < ALPHA_OPAQUE_THRESHOLD {
                    continue;
                }
                let intensity = thresholded.get_pixel(x, y)[0];
                if intensity < INK_INTENSITY_THRESHOLD {
                    dark_count += 1;
                } else {
                    light_count += 1;
                }
            }
        }

        // If dark pixels outnumber light pixels, the image has light ink on dark background
        // (like a screenshot from dark mode). In that case, the ink is the light group.
        let ink_is_dark = dark_count <= light_count;

        let rgba_image = RgbaImage::from_fn(self.width, self.height, |x, y| {
            let idx = (y as usize * self.rowstride) + (x as usize * self.n_channels);
            let orig_alpha = if has_alpha { self.data[idx + 3] } else { 255 };

            if orig_alpha < ALPHA_OPAQUE_THRESHOLD {
                return Rgba([0, 0, 0, 0]);
            }

            let pixel = thresholded.get_pixel(x, y);
            let intensity = pixel[0];

            let is_ink = if ink_is_dark {
                intensity < INK_INTENSITY_THRESHOLD
            } else {
                intensity >= INK_INTENSITY_THRESHOLD
            };

            if is_ink {
                Rgba([0, 0, 0, 255])
            } else {
                Rgba([0, 0, 0, 0])
            }
        });

        Ok(RawImageData {
            data: rgba_image.into_raw(),
            width: self.width,
            height: self.height,
            rowstride: (4 * self.width) as usize,
            n_channels: 4,
        })
    }

    // Create pixbuf from processed RGBA data (must be called on main thread)
    pub fn create_pixbuf(self) -> gdk::gdk_pixbuf::Pixbuf {
        gdk::gdk_pixbuf::Pixbuf::from_mut_slice(
            self.data,
            gdk::gdk_pixbuf::Colorspace::Rgb,
            true,
            8,
            self.width as i32,
            self.height as i32,
            self.rowstride as i32,
        )
    }
}
