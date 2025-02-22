#/usr/bin/sh
# K-Nel AB Installer
# By KeJia

ui_print '安装 K-Nel'
ui_print '安装后在模块列表可能会出现名为 "K-Nel AB Installer"的模块，无实际作用，如果出现，请卸载'
ui_print '在部分SU管理器上，脚本输出“安装成功”，而管理器输出错误，提示安装失败，这种情况无须理睬，内核已安装成功'

if test -z '$(grep alioth /system/build.prop)';
then 
  echo '设备非 红米K40 / 小米11X / POCO F3，不能安装该内核'
  exit 1
fi

cd $MODPATH/tools/
chmod +x magiskboot

ui_print '提取 Boot 镜像。。。'
dd if=/dev/block/by-name/boot_a of=$MODPATH/boot.img
ui_print '解包 Boot 镜像。。。'
./magiskboot unpack $MODPATH/boot.img
ui_print '替换内核。。。'
cp $MODPATH/Image ./kernel
cp $MODPATH/dtb ./kernel_dtb
ui_print '打包 Boot 镜像。。。'
./magiskboot repack $MODPATH/boot.img
ui_print '刷入 Boot 镜像。。。'
dd if=new-boot.img of=/dev/block/by-name/boot_a
dd if=new-boot.img of=/dev/block/by-name/boot_b
ui_print '刷入 Dtbo 镜像。。。'
dd if=$MODPATH/dtbo.img of=/dev/block/by-name/dtbo_a
dd if=$MODPATH/dtbo.img of=/dev/block/by-name/dtbo_b
ui_print '安装完成'

ui_print '清理中。。。'
rm -rf $MODPATH
