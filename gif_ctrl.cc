/* Delete local bitmap resource http://msdn.microsoft.com/en-us/library/dd183539(VS.85).aspx    */
[DllImport("gdi32.dll", CharSet = CharSet::Auto, SetLastError = true)]
extern bool DeleteObject(IntPtr hObject);

public ref class gif_control sealed : System::Windows::Controls::Image
{
	Bitmap^ bitmap_ = nullptr;
	void frame_updated_callback();
	BitmapSource^ get_bitmap_source();
public:
	/* Local bitmap member to cache image resource */        
	BitmapSource^ bitmap_source;
	delegate void FrameUpdatedEventHandler();
	void OnInitialized(EventArgs^ e) override;

	void gif_control_loaded(object sender, RoutedEventArgs^ e);
	void gif_control_unloaded(object sender, RoutedEventArgs^ e);

	void start_animate();
	void stop_animate();
	void on_frame_changed(object sender, EventArgs^ e);
};


void gif_control::OnInitialized(EventArgs^ e) {
    /* parent::OnInitialized(e);    */
    this->Loaded += gcnew RoutedEventHandler(this, &gif_control::gif_control_loaded);
    this->Unloaded += gcnew RoutedEventHandler(this, &gif_control::gif_control_unloaded);
}

/* Load the embedded image for the Image::Source    */
void gif_control::gif_control_loaded(object sender, RoutedEventArgs^ e) {
    auto resource_manager = gcnew ResourceManager("ifc_exporter.Resources", System::Reflection::Assembly::GetExecutingAssembly());
    // Get GIF image from Resources
    if (resource_manager->GetObject("IDR_ANIMATION1") != nullptr)
    {
        Width = bitmap_->Width;
        Height = bitmap_->Height;

        bitmap_source = get_bitmap_source();
        Source = bitmap_source;
    }
}

/* Close the FileStream to unlock the GIF file */
void gif_control::gif_control_unloaded(object sender, RoutedEventArgs^ e) {
    stop_animate();
}

void gif_control::start_animate() {
    ImageAnimator::Animate(bitmap_,
        gcnew EventHandler(this, &gif_control::on_frame_changed));
}

void gif_control::stop_animate() {
    ImageAnimator::StopAnimate(bitmap_,
        gcnew EventHandler(this, &gif_control::on_frame_changed));
}

/* Event handler for the frame changed  */
void gif_control::on_frame_changed(object sender, EventArgs^ e) {
    Dispatcher->BeginInvoke(DispatcherPriority::Normal,
        gcnew FrameUpdatedEventHandler(this, &gif_control::frame_updated_callback));
}

void gif_control::frame_updated_callback() {
    ImageAnimator::UpdateFrames();

    if (bitmap_source != nullptr)
        bitmap_source->Freeze();

    // Convert the bitmap to BitmapSource that can be display in WPF Visual Tree
    bitmap_source = get_bitmap_source();
    Source = bitmap_source;
    InvalidateVisual();
}

BitmapSource^ gif_control::get_bitmap_source() {
    IntPtr handle = IntPtr::Zero;

    try {
        handle = bitmap_->GetHbitmap();
        bitmap_source = System::Windows::Interop::Imaging::CreateBitmapSourceFromHBitmap(
            handle, IntPtr::Zero, Int32Rect::Empty, BitmapSizeOptions::FromEmptyOptions());
    }
    finally {
        if (handle != IntPtr::Zero)
            DeleteObject(handle);
    }

    return bitmap_source;
}