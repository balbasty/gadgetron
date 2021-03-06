#include "python_toolbox.h"
#include "GadgetronTimer.h"
#include <gtest/gtest.h>
#include "ismrmrd/ismrmrd.h"

using namespace Gadgetron;
using testing::Types;

template<typename T> class python_converter_test : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
    }
};

typedef Types<float> realImplementations;
TYPED_TEST_CASE(python_converter_test, realImplementations);

TYPED_TEST(python_converter_test, no_return_value)
{
    GDEBUG_STREAM(" --------------------------------------------------------------------------------------------------");
    GDEBUG_STREAM("Call a function with no return value (print all arguments)");

    int a = -42;
    float b = 3.141592;
    std::string c("hello, world");
    unsigned int d(117);
    std::complex<double> e(2.12894, -1.103103);

    std::vector<size_t> dims;
    dims.push_back(4);
    dims.push_back(4);
    dims.push_back(4);
    hoNDArray<std::complex<float> > arr(dims);

    PythonFunction<> foo("__builtin__", "print");
    foo(a, b, c, d, e, arr);
}

TYPED_TEST(python_converter_test, single_return_value)
{
    GDEBUG_STREAM(" --------------------------------------------------------------------------------------------------");
    GDEBUG_STREAM("Call a function with a single return value");
    PythonFunction<float> atan2("math", "atan2");
    int x = 7, y = 4;
    float atan = atan2(x, y);
    std::cout << atan << std::endl;

    EXPECT_FLOAT_EQ(atan, 1.05165);
}

TYPED_TEST(python_converter_test, tuple_return_value)
{
    GDEBUG_STREAM(" --------------------------------------------------------------------------------------------------");
    GDEBUG_STREAM("Call a function that returns a tuple");
    PythonFunction<float, float> divmod("__builtin__", "divmod");
    float w = 6.89;
    float z = 4.12;
    float fsum = 0, fdiff = 0;
    std::tie(fsum, fdiff) = divmod(w, z);
    std::cout << fsum << ", " << fdiff << std::endl;
    EXPECT_FLOAT_EQ(fsum, 1);
    EXPECT_FLOAT_EQ(fdiff, 2.77);
}

TYPED_TEST(python_converter_test, tuple_len)
{
    GDEBUG_STREAM(" --------------------------------------------------------------------------------------------------");
    GDEBUG_STREAM("Call a function that expects an iterable argument (tuple)");
    PythonFunction<int> tuplen("__builtin__", "len");
    int l = tuplen(std::make_tuple(-7, 0, 7));
    std::cout << "tuple length: " << l << std::endl;
    EXPECT_EQ(l, 3);
}

TYPED_TEST(python_converter_test, numpy_hoNDArray)
{
    GDEBUG_STREAM(" --------------------------------------------------------------------------------------------------");
    GDEBUG_STREAM("Generate an hoNDArray of even #s using numpy");
    PythonFunction<hoNDArray<float>> arange("numpy", "arange");
    hoNDArray<float> evens = arange(0, 100, 2, "f");
    std::cout << "number of even numbers between 0 and 100: " <<
        evens.get_number_of_elements() << std::endl;
    EXPECT_FLOAT_EQ(evens(0), 0);
    EXPECT_FLOAT_EQ(evens(1), 2);
    EXPECT_FLOAT_EQ(evens(2), 4);
    EXPECT_EQ(evens.get_number_of_elements(), 50);
}

TYPED_TEST(python_converter_test, ismrmrd_imageheader)
{
    {
        GILLock gl;     // this is needed
        boost::python::object main(boost::python::import("__main__"));
        boost::python::object global(main.attr("__dict__"));
        boost::python::exec("def modify(head): head.version = 42; return head",
            global, global);
    }

    GDEBUG_STREAM(" --------------------------------------------------------------------------------------------------");
    GDEBUG_STREAM("Test converter for ISMRMRD::ImageHeader");
    ISMRMRD::ImageHeader img_head, img_head2;
    img_head.version = 0;
    std::cout << "version before: " << img_head.version << std::endl;
    PythonFunction<ISMRMRD::ImageHeader> modify_img_header("__main__", "modify");
    img_head2 = modify_img_header(img_head);
    std::cout << "version after: " << img_head2.version << std::endl;
    EXPECT_EQ(img_head2.version, 42);

    GDEBUG_STREAM(" --------------------------------------------------------------------------------------------------");
    GDEBUG_STREAM("Test converter for ISMRMRD::AcquisitionHeader");
    ISMRMRD::AcquisitionHeader acq_head, acq_head2;
    acq_head.version = 0;
    std::cout << "version before: " << img_head.version << std::endl;
    PythonFunction<ISMRMRD::AcquisitionHeader> modify_acq_header("__main__", "modify");
    acq_head2 = modify_acq_header(acq_head);
    std::cout << "version after: " << acq_head2.version << std::endl;
    EXPECT_EQ(acq_head2.version, 42);
}

TYPED_TEST(python_converter_test, std_vec_complex)
{
    GDEBUG_STREAM(" --------------------------------------------------------------------------------------------------");
    GDEBUG_STREAM("Test converter for std::vector<std::complex<float>>");
    {
        GILLock gl;     // this is needed
        boost::python::object main(boost::python::import("__main__"));
        boost::python::object global(main.attr("__dict__"));
        boost::python::exec("from numpy.random import random\n"
            "def rand_cplx_array(length): \n"
            "    return random(length) + 1j * random(length)\n",
            global, global);
    }

    std::vector<std::complex<double> > vec;
    PythonFunction<std::vector<std::complex<double> > > make_vec("__main__", "rand_cplx_array");
    vec = make_vec(32);
    std::cout << vec[16] << std::endl;
    EXPECT_EQ(vec.size(), 32);
}

TYPED_TEST(python_converter_test, hoNDArray_ismrmrd_imageheader)
{
    GDEBUG_STREAM(" --------------------------------------------------------------------------------------------------");
    GDEBUG_STREAM("Test for hoNDArray<ISMRMRD::ImageHeader>")
    {
        GILLock gl;     // this is needed
        boost::python::object main(boost::python::import("__main__"));
        boost::python::object global(main.attr("__dict__"));
        boost::python::exec("import ismrmrd\n"
            "def mk_image_headers(img_head_array): \n"
            "   img_head_array[2,4].version=120\n"
            "   print img_head_array[0,0]\n"
            "   print img_head_array[2,4]\n"
            "   return img_head_array\n",
            global, global);
    }

    hoNDArray<ISMRMRD::ImageHeader> img_head_array;
    img_head_array.create(30, 10);
    for (int n = 0; n < img_head_array.get_number_of_elements(); n++)
        img_head_array(n).version = 345;

    GDEBUG_STREAM(" --------------------------------------------------------------------------------------------------");
    GDEBUG_STREAM("Test converter for PythonFunction<hoNDArray<ISMRMRD::ImageHeader> >");
    PythonFunction<hoNDArray<ISMRMRD::ImageHeader> > make_image_header("__main__", "mk_image_headers");
    img_head_array = make_image_header(img_head_array);
    std::cout << img_head_array(2, 4).version << std::endl;
    EXPECT_EQ(img_head_array(2, 4).version, 120);
    EXPECT_EQ(img_head_array(1, 4).version, 345);
    EXPECT_EQ(img_head_array.get_size(0), 30);
    EXPECT_EQ(img_head_array.get_size(1), 10);
}

TYPED_TEST(python_converter_test, ismrmrd_meta)
{
    GDEBUG_STREAM(" --------------------------------------------------------------------------------------------------");
    GDEBUG_STREAM("Test for ISMRMRD::MetaContainer")
    {
        GILLock gl;     // this is needed
        boost::python::object main(boost::python::import("__main__"));
        boost::python::object global(main.attr("__dict__"));
        boost::python::exec("import ismrmrd\n"
            "def mk_meta(meta): \n"
            "   mt = ismrmrd.Meta.deserialize(meta)\n"
            "   print mt['TestLong']\n"
            "   print mt['TestDouble']\n"
            "   print mt['TestString']\n"
            "   mt_str = ismrmrd.Meta.serialize(mt)\n"
            "   return mt_str\n",
            global, global);
    }

    ISMRMRD::MetaContainer meta;
    meta.set("TestLong", (long)1);
    meta.append("TestLong", (long)2);
    meta.append("TestLong", (long)3);

    meta.set("TestDouble", 1.0);
    meta.append("TestDouble", 2.1);
    meta.append("TestDouble", 3.2);

    meta.set("TestString", "This");
    meta.append("TestString", "is");
    meta.append("TestString", "a test!");

    PythonFunction<ISMRMRD::MetaContainer> make_meta("__main__", "mk_meta");
    ISMRMRD::MetaContainer meta_res = make_meta(meta);
    std::stringstream meta_res_str;
    ISMRMRD::serialize(meta_res, meta_res_str);
    GDEBUG_STREAM(meta_res_str.str());

    EXPECT_EQ(meta.as_long("TestLong", 0), 1);
    EXPECT_EQ(meta.as_long("TestLong", 1), 2);
    EXPECT_EQ(meta.as_long("TestLong", 2), 3);

    EXPECT_FLOAT_EQ(meta.as_double("TestDouble", 0), 1.0);
    EXPECT_FLOAT_EQ(meta.as_double("TestDouble", 1), 2.1);
    EXPECT_FLOAT_EQ(meta.as_double("TestDouble", 2), 3.2);
}

TYPED_TEST(python_converter_test, vec_ismrmrd_meta)
{
    GDEBUG_STREAM(" --------------------------------------------------------------------------------------------------");
    GDEBUG_STREAM("Test converter for std::vector<ISMRMRD::MetaContainer>");
    {
        GILLock gl;     // this is needed
        boost::python::object main(boost::python::import("__main__"));
        boost::python::object global(main.attr("__dict__"));
        boost::python::exec("import ismrmrd\n"
            "def mk_vector_meta(meta_str): \n"
            "   print(len(meta_str))\n"
            "   mt = list()\n"
            "   for x in meta_str:\n"
            "       mt.append(ismrmrd.Meta.deserialize(x))\n"
            "   print mt[0]['TestLong']\n"
            "   print mt[0]['TestDouble']\n"
            "   print mt[0]['TestString']\n"
            "   print mt[11]['TestLong']\n"
            "   print mt[11]['TestDouble']\n"
            "   print mt[11]['TestString']\n"
            "   res_str = list()\n"
            "   for x in mt:\n"
            "       res_str_curr=ismrmrd.Meta.serialize(x)\n"
            "       res_str.append(res_str_curr)\n"
            "   return res_str\n",
            global, global);
    }

    std::vector<ISMRMRD::MetaContainer> meta(12);

    for (int n = 0; n < meta.size(); n++)
    {
        meta[n].set("TestLong", (long)1 * n);
        meta[n].append("TestLong", (long)2 * n);
        meta[n].append("TestLong", (long)3 * n);

        meta[n].set("TestDouble", 1.0 * n);
        meta[n].append("TestDouble", 2.1 * n);
        meta[n].append("TestDouble", 3.2 * n);

        meta[n].set("TestString", "This");
        meta[n].append("TestString", "is");
        meta[n].append("TestString", "a test!");
    }

    PythonFunction< std::vector<ISMRMRD::MetaContainer> > mk_vector_meta("__main__", "mk_vector_meta");
    std::vector<ISMRMRD::MetaContainer> meta_res = mk_vector_meta(meta);

    for (int n = 0; n < meta.size(); n++)
    {
        GDEBUG_STREAM("Meta data : " << n);
        GDEBUG_STREAM("-------------------------------------------------");
        std::stringstream meta_res_str;
        ISMRMRD::serialize(meta_res[n], meta_res_str);
        // GDEBUG_STREAM(meta_res_str.str());

        EXPECT_EQ(meta[n].as_double("TestLong", 0), 1 * n);
        EXPECT_EQ(meta[n].as_double("TestLong", 1), 2 * n);
        EXPECT_EQ(meta[n].as_double("TestLong", 2), 3 * n);

        EXPECT_FLOAT_EQ(meta[n].as_double("TestDouble", 0), 1.0*n);
        EXPECT_FLOAT_EQ(meta[n].as_double("TestDouble", 1), 2.1*n);
        EXPECT_FLOAT_EQ(meta[n].as_double("TestDouble", 2), 3.2*n);

        EXPECT_STREQ(meta[n].as_str("TestString", 0), "This");
        EXPECT_STREQ(meta[n].as_str("TestString", 1), "is");
        EXPECT_STREQ(meta[n].as_str("TestString", 2), "a test!");
    }
}

TYPED_TEST(python_converter_test, ismrmrd_image_array)
{
    char* gt_home = std::getenv("GADGETRON_HOME");
    if (gt_home != NULL)
    {
        size_t pos = std::string(gt_home).rfind("gadgetron");
        gt_home[pos - 1] = '\0';
        std::string path_name = std::string(gt_home) + std::string("/share/gadgetron/python");

        std::string add_path_cmd = std::string("import sys;\nsys.path.insert(0, \"") + path_name + std::string("\")\n");
        GDEBUG_STREAM(add_path_cmd);

        GILLock gl;
        boost::python::exec(add_path_cmd.c_str(),
            boost::python::import("__main__").attr("__dict__"));
    }

    if (gt_home != NULL)
    {
        GDEBUG_STREAM(" --------------------------------------------------------------------------------------------------");
        GDEBUG_STREAM("Test converter for ISDMRMRD::IsmrmrdImageArray");

        {
            GILLock gl;     // this is needed
            boost::python::object main(boost::python::import("__main__"));
            boost::python::object global(main.attr("__dict__"));
            boost::python::exec("import ismrmrd\n"
                "def mk_ismrmrd_image_array(array_data): \n"
                "   print(array_data.data.shape)\n"
                "   print(array_data.data[128, 56, 0, 12, 3, 4, 1])\n"
                "   print(array_data.headers[3, 4, 0])\n"
                "   mt = list()\n"
                "   for x in array_data.meta:\n"
                "       curr_meta = ismrmrd.Meta.deserialize(x)\n"
                "       curr_meta['TestString'][0]='Gadgetron'\n"
                "       mt.append(curr_meta)\n"
                "   array_data.headers[1, 2, 0].version=12345\n"
                "   res_str = list()\n"
                "   for x in mt:\n"
                "       res_str_curr=ismrmrd.Meta.serialize(x)\n"
                "       res_str.append(res_str_curr)\n"
                "   array_data.meta=res_str\n"
                "   return array_data\n",
                global, global);
        }

        Gadgetron::IsmrmrdImageArray array_data;
        array_data.data_.create(192, 144, 1, 32, 4, 5, 2); // [RO E1 E2 CHA N S SLC]
        array_data.headers_.create(4, 5, 2);
        array_data.meta_.resize(4 * 5 * 2);

        size_t n;
        for (n = 0; n < array_data.data_.get_number_of_elements(); n++)
        {
            array_data.data_(n) = std::complex<float>(3.0, 124.2);
        }

        memset(array_data.headers_.get_data_ptr(), 0, sizeof(ISMRMRD::ImageHeader) * 8);

        for (n = 0; n < array_data.headers_.get_number_of_elements(); n++)
        {
            array_data.headers_(n).version = 123;
        }

        for (int n = 0; n < 4 * 5 * 2; n++)
        {
            array_data.meta_[n].set("TestLong", (long)1 * n);
            array_data.meta_[n].append("TestLong", (long)2 * n);
            array_data.meta_[n].append("TestLong", (long)3 * n);

            array_data.meta_[n].set("TestDouble", 1.0 * n);
            array_data.meta_[n].append("TestDouble", 2.1 * n);
            array_data.meta_[n].append("TestDouble", 3.2 * n);

            array_data.meta_[n].set("TestString", "This");
            array_data.meta_[n].append("TestString", "is");
            array_data.meta_[n].append("TestString", "a test!");
        }

        PythonFunction< Gadgetron::IsmrmrdImageArray > mk_ismrmrd_image_array("__main__", "mk_ismrmrd_image_array");
        Gadgetron::IsmrmrdImageArray array_res = mk_ismrmrd_image_array(array_data);

        GDEBUG_STREAM(array_data.data_(65558));
        GDEBUG_STREAM(array_data.headers_(2, 2, 0).version);
        GDEBUG_STREAM(array_data.headers_(1, 2, 0).version);

        EXPECT_FLOAT_EQ(array_data.data_(65558).real(), 3.0);
        EXPECT_FLOAT_EQ(array_data.data_(65558).imag(), 124.2);
        EXPECT_EQ(array_data.headers_(2, 2, 0).version, 123);
        EXPECT_EQ(array_data.headers_(1, 2, 0).version, 123);

        std::stringstream meta_res_str;
        ISMRMRD::serialize(array_res.meta_[6], meta_res_str);
        GDEBUG_STREAM(meta_res_str.str());

        EXPECT_STREQ(array_res.meta_[6].as_str("TestString", 0), "Gadgetron");
        EXPECT_STREQ(array_res.meta_[5].as_str("TestString", 0), "Gadgetron");
        EXPECT_STREQ(array_res.meta_[5].as_str("TestString", 1), "is");
        EXPECT_STREQ(array_res.meta_[5].as_str("TestString", 2), "a test!");
    }
}
