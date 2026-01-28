use crate::deps::*;
use ink_stroke_modeler_rs::{ModelerInput, ModelerInputEventType, ModelerParams, StrokeModeler};
use papers_document::{AnnotationInk, InkList, Path, Point};
use papers_view::AnnotationsContext;

// Smooths stroke points using ink-stroke-modeler.
pub fn smooth_stroke_points(
    points: Vec<(f64, f64)>,
    timestamps: Option<Vec<f64>>,
    params: ModelerParams,
    pressure: f64,
) -> Result<Vec<(f64, f64)>, String> {
    if points.len() < 2 {
        return Ok(points);
    }

    let mut modeler = StrokeModeler::new(params)
        .map_err(|e| format!("Failed to create stroke modeler: {}", e))?;

    // Build inputs, initially all set to Move
    let mut input: Vec<ModelerInput> = points
        .iter()
        .enumerate()
        .map(|(i, (x, y))| ModelerInput {
            event_type: ModelerInputEventType::Move,
            pos: (*x, *y),
            time: timestamps
                .as_ref()
                .and_then(|ts| ts.get(i).copied())
                .unwrap_or(i as f64 * 0.01),
            pressure,
        })
        .collect();

    // Set proper event types for first and last points
    let n = input.len();
    if let Some(first) = input.first_mut() {
        first.event_type = ModelerInputEventType::Down;
    }
    if let Some(last) = input.get_mut(n - 1) {
        last.event_type = ModelerInputEventType::Up;
    }

    // Process all inputs and collect smoothed points
    let result_stroke: Vec<(f64, f64)> = input
        .into_iter()
        .filter_map(|i| modeler.update(i).ok())
        .flatten()
        .map(|r| r.pos)
        .collect();

    if result_stroke.is_empty() {
        Ok(points)
    } else {
        Ok(result_stroke)
    }
}

pub fn setup() {
    AnnotationsContext::register_ink_transformation(|a| {
        if let Ok(ink) = a.clone().downcast::<AnnotationInk>() {
            let mut p = ModelerParams::suggested();
            p.sampling_max_outputs_per_call = 200;

            let time_list = ink.time_list();
            let t0 = if let Some(k) = time_list.first() {
                k.time() as i32
            } else {
                return;
            };

            // Extract points and timestamps from time_list
            let points: Vec<(f64, f64)> = time_list.iter().map(|t| (t.x(), t.y())).collect();
            let timestamps: Vec<f64> = time_list
                .iter()
                .map(|t| ((t.time() as i32 - t0) as f64) / 1000.)
                .collect();

            // Smooth the stroke using the shared helper
            let result_stroke = match smooth_stroke_points(points, Some(timestamps), p, 0.25) {
                Ok(smoothed) => smoothed,
                Err(e) => {
                    eprintln!("modeler error: {e}");
                    return;
                }
            };
            let area: (f64, f64, f64, f64) = result_stroke
                .iter()
                .fold(None, |a, p| {
                    let b = a.unwrap_or((p.0, p.0, p.1, p.1));
                    Some((b.0.min(p.0), b.1.max(p.0), b.2.min(p.1), b.3.max(p.1)))
                })
                .unwrap();

            let r = a.border_width();
            let is_point = (area.0 - area.1).powf(2.) + (area.2 - area.3).powf(2.) < r;
            let ip = if is_point {
                let mut p1 = Point::new();
                let mut p2 = Point::new();
                p1.set_x(area.0 - r / 2.);
                p1.set_y(area.2 - r / 2.);
                p2.set_x(area.0 + r / 2.);
                p2.set_y(area.2 + r / 2.);
                Path::for_array(&[p1, p2])
            } else {
                let result_simplified = {
                    use geo::Simplify;
                    use geo_types::LineString;
                    let result_string: LineString = result_stroke.into();
                    let simplified = result_string.simplify(&0.05);

                    simplified
                        .points()
                        .map(|r| {
                            let mut p = Point::new();
                            p.set_x(r.x());
                            p.set_y(r.y());
                            p
                        })
                        .collect::<Vec<Point>>()
                };
                Path::for_array(&result_simplified)
            };

            let il = InkList::for_array(&[ip]);
            ink.set_ink_list(il);
        }
    });
}
