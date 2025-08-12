use glib::Object;

use crate::deps::*;

mod imp {
    use graphene::Size;
    use std::cell::Cell;

    use gtk::gsk;

    use super::*;

    #[derive(Properties)]
    #[properties(wrapper_type = super::SimpleColorSwatch)]
    pub struct SimpleColorSwatch {
        #[property(get, set)]
        color: Cell<gdk::RGBA>,
    }

    impl Default for SimpleColorSwatch {
        fn default() -> Self {
            SimpleColorSwatch {
                color: Cell::new(gdk::RGBA::BLACK),
            }
        }
    }

    #[glib::object_subclass]
    impl ObjectSubclass for SimpleColorSwatch {
        const NAME: &'static str = "SimpleColorSwatch";
        type Type = super::SimpleColorSwatch;
        type ParentType = gtk::Widget;
    }

    #[glib::derived_properties]
    impl ObjectImpl for SimpleColorSwatch {
        fn constructed(&self) {
            self.parent_constructed();

            self.obj().connect_color_notify(glib::clone!(
                #[weak(rename_to = obj)]
                self,
                move |_| {
                    obj.obj().queue_draw();
                }
            ));
        }
    }

    impl WidgetImpl for SimpleColorSwatch {
        fn snapshot(&self, snapshot: &gtk::Snapshot) {
            let w = self.obj().width() as f32;
            let h = self.obj().height() as f32;
            let r = if w <= h { w } else { h };
            let rsize = Size::new(r, r);
            let bounds = graphene::Rect::new((w - r) / 2., (h - r) / 2., r, r);
            snapshot.push_rounded_clip(&gsk::RoundedRect::new(bounds, rsize, rsize, rsize, rsize));

            /* Most document have a white background, so some white
            is drawn in the background in case self.color has some alpha */
            snapshot.append_color(&gdk::RGBA::WHITE, &bounds);
            snapshot.append_color(&self.color.get(), &bounds);
            snapshot.pop();
        }
    }
}

glib::wrapper! {
    pub struct SimpleColorSwatch(ObjectSubclass<imp::SimpleColorSwatch>)
        @extends gtk::Widget,
        @implements gtk::Accessible, gtk::Actionable, gtk::Buildable, gtk::ConstraintTarget;
}

impl SimpleColorSwatch {
    pub fn new() -> Self {
        Object::builder().build()
    }
}
impl Default for SimpleColorSwatch {
    fn default() -> Self {
        Self::new()
    }
}
