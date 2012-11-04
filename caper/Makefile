.SUFFIXES: .o .c .cpp .d
CXX			= g++ -std=c++11 -pedantic -Wall -Wextra -Wno-unused-parameter -DLINUX -O3 --input-charset=cp932
TARGET		= caper
OBJS		= $(TARGET).o caper_cpg.o caper_tgt.o caper_generate_cpp.o caper_generate_d.o caper_generate_csharp.o caper_generate_js.o caper_generate_java.o
#TARGET		= grammar_test
#OBJS		= grammar_test.o
DEPENDDIR	= ./depend

VPATH = ./depend

.c.d:
	@$(CXX) -MM $< > $(DEPENDDIR)/$*.d.t
	@cp $(DEPENDDIR)/$*.d.t $(DEPENDDIR)/$*.d
	@sed -e 's/$*\.o/$*\.d/g' $(DEPENDDIR)/$*.d.t >> $(DEPENDDIR)/$*.d
	@rm $(DEPENDDIR)/$*.d.t
	@echo '$*.o : $*.d' >> $(DEPENDDIR)/$*.d	

.cpp.d:
	@$(CXX) -MM $< > $(DEPENDDIR)/$*.d.t
	@cp $(DEPENDDIR)/$*.d.t $(DEPENDDIR)/$*.d
	@sed -e 's/$*\.o/$*\.d/g' $(DEPENDDIR)/$*.d.t >> $(DEPENDDIR)/$*.d
	@rm $(DEPENDDIR)/$*.d.t
	@echo '$*.o : $*.d' >> $(DEPENDDIR)/$*.d	

.cpp.o:
	$(CXX) -g -c -o $@ $<

.c.o:
	$(CXX) -g -c -o $@ $<

all:
	mkdir -p depend
	make depend -s
	make $(TARGET)

depend: $(OBJS:.o=.d)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^

clean:
	rm -f $(TARGET) $(OBJS)
	rm -rf $(DEPENDDIR)

publish:
	rm -rf /tmp/caper /tmp/caper-*
	svn export . /tmp/caper
	rm -f ~/caper-`date +%Y-%m-%d`.zip
	cd /tmp; zip -r9 /home/naoyuki/caper/caper/site/caper-`date +%Y-%m-%d`.zip caper

-include $(addprefix $(DEPENDDIR)/,$(OBJS:.o=.d))
