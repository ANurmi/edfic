DUT = edfic_top
TB  = $(VERIL_DIR)/TbEdfIc.cpp

.PHONY: lint
lint:
	verilator \
	--lint-only \
	$(shell bender script flist) \
	--cc $(TB)

.PHONY: verilate
verilate:
	verilator \
	$(shell bender script flist) \
	--cc $(TB) \
	--build --exe \
	-O3 \
	-sv \
	--timing \
	--x-assign unique \
	--trace-fst \
	--trace-structs \
	--trace-params \
	--top-module $(DUT) \
	-Mdir $(BUILD_DIR)/obj_dir

.PHONY: simv
simv:
	$(BUILD_DIR)/obj_dir/V$(DUT)
