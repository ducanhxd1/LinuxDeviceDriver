# Lab 02 — Module Parameters

Module nhận `repeat`, `enabled` và `message`. Init validate `repeat` trong 1–10;
giá trị sai trả `-EINVAL`. Permission 0444 cho phép quan sát dưới sysfs nhưng
không sửa runtime.

```sh
make && make test
sudo insmod build/ldd_params.ko repeat=3 enabled=1 message='hello learner'
cat /sys/module/ldd_params/parameters/{repeat,enabled,message}
sudo rmmod ldd_params
```

Thử `repeat=0` và chứng minh module không được giữ lại sau init failure.

