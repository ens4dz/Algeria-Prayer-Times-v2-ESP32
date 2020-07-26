# Algeria-Prayer-Times-v2-ESP32
Salat clock

https://www.youtube.com/watch?v=DgoBRQLJM6M

.السلام عليكم، الهدف من المشروع تصميم ساعة حديثة اقتصادية وقابلة للتخصيص، تعرض أحاديث أو أذكار أو مواقيت المحاضرات

مميزات الساعة:
تكلفة منخفضة
قابلية التغيير الشكل في أي وقت
سهل النقل والتركيب
![salat](https://raw.githubusercontent.com/ens4dz/Algeria-Prayer-Times-v2-ESP32/master/esp32-salat.png)
![vga_pins](https://raw.githubusercontent.com/ens4dz/Salat-Time-esp32/master/vga_pins.jpg)
![esp32_vga_rtc_pins](https://raw.githubusercontent.com/ens4dz/Algeria-Prayer-Times-v2-ESP32/master/esp32-vga-rtc.jpg)
## الخصائص الحالية:
- [x] طريقة حساب الوقت مطابقة لوزارة الشؤون الدينية للجزائر .
- [x] تدعم 60 مدينة جزائرية حسب التطبيق الرسمي للوزارة
- [x] تقليص قاعدة البيانات (تحسن ملحوظ من7 ميغا أصبحت 1 ميغا)
- [x] اضافة مؤقتة تعمل ببطارية صغيرة، لتفادي ضياع التوقيت عند انقطاع الكهرباء 
- [x] امكانية تغيير المدينة باستخدام الهاتف 
- [x] سهولة التطبيق : شاشة و rtc , esp32 

![wifi-config](https://raw.githubusercontent.com/ens4dz/Algeria-Prayer-Times-v2-ESP32/master/wifi_config.jpg)



## مزايا ناقصة:
- [ ] اضافة اعلانات مثل #أغلق_هاتفك بين الأذان والإقامة الصلاة
- [ ] عرض رسائل من الهاتف الى الشاشة 
- [ ] تحديث قاعدة البيانات تلقائيا بحلول عام هجري جديد
- [ ] اضافة تغيير الخطوط وأماكن العرض من الهاتف

## المكتبات المستخدمة:

### VGA:
1-https://github.com/bitluni/ESP32Lib 
(GfxWrapper.h needs a patch for rotation )

2-https://github.com/adafruit/Adafruit-GFX-Library


### DB:

3-https://github.com/siara-cc/esp32_arduino_sqlite3_lib


### Rtc time:

4-https://github.com/Makuna/Rtc//clock 


### WEB server and OTA:

5-https://github.com/ayushsharma82/AsyncElegantOTA

6-https://github.com/me-no-dev/ESPAsyncWebServer

7-https://github.com/me-no-dev/AsyncTCP

إذا أعجبتك الفكرة ساهم في تحسينها 
