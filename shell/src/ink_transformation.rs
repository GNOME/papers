use crate::deps::*;
use ink_stroke_modeler_rs::{ModelerInput, ModelerInputEventType, ModelerParams, StrokeModeler};
use papers_document::{AnnotationInk, InkList, Path, Point};
use papers_view::AnnotationsContext;

pub fn setup() {
    AnnotationsContext::register_ink_transformation(|a| {
        if let Ok(ink) = a.clone().downcast::<AnnotationInk>() {
            let mut p = ModelerParams::suggested();
            p.sampling_max_outputs_per_call = 200;

            let mut modeler = StrokeModeler::new(p).expect("modeler");

            let time_list = ink.time_list();
            let t0 = if let Some(k) = time_list.first() {
                k.time() as i32
            } else {
                return;
            };
            let mut input: Vec<ModelerInput> = time_list
                .into_iter()
                .map(|t| ModelerInput {
                    event_type: ModelerInputEventType::Move,
                    pos: (t.x(), t.y()),
                    time: ((t.time() as i32 - t0) as f64) / 1000.,
                    pressure: 0.25,
                })
                .collect();
            let n = input.len();
            if let Some(k) = input.get_mut(n - 1) {
                k.event_type = ModelerInputEventType::Up;
            }
            if let Some(k) = input.first_mut() {
                k.event_type = ModelerInputEventType::Down;
            }

            let mut must_exit = false;

            let result_stroke = input
                .into_iter()
                .filter_map(|i| {
                    modeler
                        .update(i)
                        .map_err(|e| {
                            must_exit = true;
                            eprintln!("modeler updated, Err: {e:?}")
                        })
                        .ok()
                })
                .flatten()
                .map(|r| r.pos)
                .collect::<Vec<(f64, f64)>>();
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

            if must_exit {
                return;
            }
            let il = InkList::for_array(&[ip]);
            ink.set_ink_list(il);
        }
    });
}
