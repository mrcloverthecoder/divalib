#include <stdio.h>
#include <Python.h>
#include <structmember.h>
#include <diva_auth3d.h>

#define RETURN_NONE_REF Py_INCREF(Py_None); return Py_None
#define PYSTR_INIT(obj, s, data) obj->s = PyUnicode_FromString(data); \
								 if (obj->s == nullptr) { Py_DECREF(obj); return nullptr; }

static std::string Narrow(PyObject* str)
{
	Py_ssize_t size;
	const char* ptr = PyUnicode_AsUTF8AndSize(str, &size);
	if (ptr != nullptr)
	{
		std::string s(ptr);
		return std::move(s);
	}

	return std::string();
}

struct AuthClassObject
{
	PyObject_HEAD
	PyObject* Objects;
	PyObject* ObjectNameList;
	PyObject* ObjectHrcList;
	PyObject* ObjectHrcNameList;
	float Begin, FPS, Size;
};

struct ObjhrcClassObject
{
	PyObject_HEAD
	PyObject* Name;
	PyObject* UIDName;
	PyObject* NodeList;
};

struct Property1DClassObject
{
	PyObject_HEAD
	int32_t Type;
	float Value;
	PyObject* Keys;
};

struct Property3DClassObject
{
	PyObject_HEAD
	PyObject *X, *Y, *Z;
};

struct HrcNodeClassObject
{
	PyObject_HEAD
	PyObject* Name;
	int32_t Parent;
	PyObject *Trans, *Rot, *Scale, *Visibility;
};

struct ObjectClassObject
{
	PyObject_HEAD
	PyObject *Name, *UIDName;
	PyObject *Trans, *Rot, *Scale, *Visibility;
};

static PyTypeObject AuthClassType = { PyVarObject_HEAD_INIT(NULL, 0) };
static PyTypeObject ObjhrcClassType = { PyVarObject_HEAD_INIT(NULL, 0) };
static PyTypeObject Property1DClassType = { PyVarObject_HEAD_INIT(NULL, 0) };
static PyTypeObject Property3DClassType = { PyVarObject_HEAD_INIT(NULL, 0) };
static PyTypeObject HrcNodeClassType = { PyVarObject_HEAD_INIT(NULL, 0) };
static PyTypeObject ObjectClassType = { PyVarObject_HEAD_INIT(NULL, 0) };

// TYPE FUNCTIONS
static PyObject* Auth3d_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	AuthClassObject* self;
	self = (AuthClassObject*)type->tp_alloc(type, 0);
	if (self == nullptr)
		return nullptr;

	self->ObjectHrcList = PyList_New(0);
	self->ObjectHrcNameList = PyList_New(0);
	self->Objects = PyList_New(0);
	self->ObjectNameList = PyList_New(0);

	return (PyObject*)self;
}

static int Property1D_init(Property1DClassObject* self, PyObject* args, PyObject* kwds)
{
	self->Type = 0;
	self->Value = 0.0f;
	self->Keys = PyList_New(0);
	return 0;
}

static PyObject* ObjectHrc_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	ObjhrcClassObject* self;
	self = (ObjhrcClassObject*)type->tp_alloc(type, 0);
	if (self == nullptr)
		return nullptr;

	PYSTR_INIT(self, Name, "NO_NAME");
	PYSTR_INIT(self, UIDName, "NO_NAME");
	self->NodeList = PyList_New(0);

	return (PyObject*)self;
}

static PyObject* Property3d_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	Property3DClassObject* self;
	self = (Property3DClassObject*)type->tp_alloc(type, 0);
	if (self == nullptr)
		return nullptr;

	self->X = PyObject_CallObject((PyObject*)&Property1DClassType, nullptr);
	self->Y = PyObject_CallObject((PyObject*)&Property1DClassType, nullptr);
	self->Z = PyObject_CallObject((PyObject*)&Property1DClassType, nullptr);

	return (PyObject*)self;
}

static void InitScaleVisibilityObject(PyObject* scl, PyObject* vis)
{
	auto* scale = (Property3DClassObject*)scl;
	auto* visibility = (Property1DClassObject*)vis;
	((Property1DClassObject*)scale->X)->Type = Auth::KEY_TYPE_STATIC;
	((Property1DClassObject*)scale->X)->Value = 1.0f;
	((Property1DClassObject*)scale->Y)->Type = Auth::KEY_TYPE_STATIC;
	((Property1DClassObject*)scale->Y)->Value = 1.0f;
	((Property1DClassObject*)scale->Z)->Type = Auth::KEY_TYPE_STATIC;
	((Property1DClassObject*)scale->Z)->Value = 1.0f;
	visibility->Type = Auth::KEY_TYPE_STATIC;
	visibility->Value = 1.0f;
}

static PyObject* HrcNode_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	HrcNodeClassObject* self;
	self = (HrcNodeClassObject*)type->tp_alloc(type, 0);
	if (self == nullptr)
		return nullptr;

	PYSTR_INIT(self, Name, "NO_NAME");
	self->Parent = -1;
	self->Trans = PyObject_CallObject((PyObject*)&Property3DClassType, nullptr);
	self->Rot = PyObject_CallObject((PyObject*)&Property3DClassType, nullptr);
	self->Scale = PyObject_CallObject((PyObject*)&Property3DClassType, nullptr);
	self->Visibility = PyObject_CallObject((PyObject*)&Property1DClassType, nullptr);

	// NOTE: Set scale and visibility to 1 by default
	InitScaleVisibilityObject(self->Scale, self->Visibility);

	return (PyObject*)self;
}

static PyObject* Object_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	ObjectClassObject* self;
	self = (ObjectClassObject*)type->tp_alloc(type, 0);
	if (self == nullptr)
		return nullptr;

	PYSTR_INIT(self, Name, "NO_NAME");
	PYSTR_INIT(self, UIDName, "NO_UID");
	self->Trans = PyObject_CallObject((PyObject*)&Property3DClassType, nullptr);
	self->Rot = PyObject_CallObject((PyObject*)&Property3DClassType, nullptr);
	self->Scale = PyObject_CallObject((PyObject*)&Property3DClassType, nullptr);
	self->Visibility = PyObject_CallObject((PyObject*)&Property1DClassType, nullptr);

	// NOTE: Set scale and visibility to 1 by default
	InitScaleVisibilityObject(self->Scale, self->Visibility);

	return (PyObject*)self;
}

static void Auth3d_dealloc(AuthClassObject* self)
{
	Py_TYPE(self)->tp_free((PyObject*)self);
}

// TYPE MEMBERS
static PyMemberDef Auth3d_members[] = {
	{ "objhrc_list", T_OBJECT, offsetof(AuthClassObject, ObjectHrcList), 0, "List of objhrc." },
	{ "objhrc_name_list", T_OBJECT, offsetof(AuthClassObject, ObjectHrcNameList), 0, "List of objhrc names." },
	{ "object_list", T_OBJECT, offsetof(AuthClassObject, Objects), 0, "List of object." },
	{ "object_name_list", T_OBJECT, offsetof(AuthClassObject, ObjectNameList), 0, "List of object names." },
	{ "begin", T_FLOAT, offsetof(AuthClassObject, Begin), 0, "Begin frame" },
	{ "fps", T_FLOAT, offsetof(AuthClassObject, FPS), 0, "Framerate" },
	{ "size", T_FLOAT, offsetof(AuthClassObject, Size), 0, "End frame" },
	{ nullptr }
};

static PyMemberDef ObjectHrc_members[] = {
	{ "name", T_OBJECT, offsetof(ObjhrcClassObject, Name), 0, "Name of the HRC object." },
	{ "uid_name", T_OBJECT, offsetof(ObjhrcClassObject, UIDName), 0, "UID of the HRC object." },
	{ "node_list", T_OBJECT, offsetof(ObjhrcClassObject, NodeList), 0, "List of HRC nodes." },
	{ nullptr }
};

static PyMemberDef HrcNode_members[] = {
	{ "name", T_OBJECT, offsetof(HrcNodeClassObject, Name), 0, "Name of the node." },
	{ "parent", T_INT, offsetof(HrcNodeClassObject, Parent), 0, "Parent node index. -1 for none." },
	{ "trans", T_OBJECT, offsetof(HrcNodeClassObject, Trans), 0, "Traslation of the node." },
	{ "rot", T_OBJECT, offsetof(HrcNodeClassObject, Rot), 0, "Rotation of the node." },
	{ "scale", T_OBJECT, offsetof(HrcNodeClassObject, Scale), 0, "Scale of the node." },
	{ "visibility", T_OBJECT, offsetof(HrcNodeClassObject, Visibility), 0, "Visibility of the node." },
	{ nullptr }
};

static PyMemberDef Object_members[] = {
	{ "name", T_OBJECT, offsetof(ObjectClassObject, Name), 0, "Name of the object." },
	{ "uid_name", T_OBJECT, offsetof(ObjectClassObject, UIDName), 0, "UID of the object." },
	{ "trans", T_OBJECT, offsetof(ObjectClassObject, Trans), 0, "Traslation of the object." },
	{ "rot", T_OBJECT, offsetof(ObjectClassObject, Rot), 0, "Rotation of the object." },
	{ "scale", T_OBJECT, offsetof(ObjectClassObject, Scale), 0, "Scale of the object." },
	{ "visibility", T_OBJECT, offsetof(ObjectClassObject, Visibility), 0, "Visibility of the object." },
	{ nullptr }
};

static PyMemberDef Property1d_members[] = {
	{ "type", T_INT, offsetof(Property1DClassObject, Type), 0, "Key type." },
	{ "value", T_FLOAT, offsetof(Property1DClassObject, Value), 0, "Static key value." },
	{ "key_list", T_OBJECT, offsetof(Property1DClassObject, Keys), 0, "Keyframe list." },
	{ nullptr }
};

static PyMemberDef Property3d_members[] = {
	{ "x", T_OBJECT, offsetof(Property3DClassObject, X), 0, "X axis." },
	{ "y", T_OBJECT, offsetof(Property3DClassObject, Y), 0, "Y axis." },
	{ "z", T_OBJECT, offsetof(Property3DClassObject, Z), 0, "Z axis." },
	{ nullptr }
};

static void ConvertProperty1D(Property1DClassObject* src, Auth::Property1D& dst)
{
	dst.Type = src->Type;
	dst.Value = src->Value;
	// NOTE: Allocate space for the keyframes
	size_t frameCount = (size_t)PyList_Size(src->Keys);
	dst.Keys.reserve(frameCount);

	for (Py_ssize_t i = 0; i < PyList_Size(src->Keys); i++)
	{
		auto* key = PyList_GetItem(src->Keys, i);
		Py_ssize_t keyListSize = PyList_Size(key);
		if (keyListSize < 5)
			continue;

		dst.AddKey(
			PyLong_AsLong(PyList_GetItem(key, 0)),
			(float)PyFloat_AsDouble(PyList_GetItem(key, 1)),
			(float)PyFloat_AsDouble(PyList_GetItem(key, 2)),
			(float)PyFloat_AsDouble(PyList_GetItem(key, 3)),
			(float)PyFloat_AsDouble(PyList_GetItem(key, 4))
		);
	}
}

static void ConvertProperty3D(Property3DClassObject* src, Auth::Property3D& dst)
{
	ConvertProperty1D((Property1DClassObject*)src->X, dst.X);
	ConvertProperty1D((Property1DClassObject*)src->Y, dst.Y);
	ConvertProperty1D((Property1DClassObject*)src->Z, dst.Z);
}

// TYPE METHODS
// auth3d.Auth3d (AuthClassType)

enum ExportFormat
{
	EXPORT_FORMAT_A3DA = 0,
	EXPORT_FORMAT_A3DC
};

static PyObject* AuthClassType_Write(AuthClassObject* self, PyObject* args)
{
	char buffer[0x200] = { '\0' };
	const char* file = "";
	int32_t format = 0;
	int32_t compress = 0;
	if (!PyArg_ParseTuple(args, "sii", &file, &format, &compress))
		return nullptr;

	if (compress < 0 || compress > 2)
		compress = 0;

	Auth::Auth3D a3d = { };
	a3d.Filename = IO::Path::GetFilename(file);
	a3d.PlayControl.Begin = self->Begin;
	a3d.PlayControl.Framerate = self->FPS;
	a3d.PlayControl.Size = self->Size;

	for (Py_ssize_t i = 0; i < PyList_Size(self->ObjectHrcList); i++)
	{
		auto* src = (ObjhrcClassObject*)PyList_GetItem(self->ObjectHrcList, i);
		auto& hrc = a3d.ObjectHrcs.emplace_back();

		hrc.Name = Narrow(src->Name);
		hrc.UIDName = Narrow(src->UIDName);
		for (Py_ssize_t j = 0; j < PyList_Size(src->NodeList); j++)
		{
			auto* srcNode = (HrcNodeClassObject*)PyList_GetItem(src->NodeList, j);
			auto& node = hrc.Nodes.emplace_back();

			node.Name = Narrow(srcNode->Name);
			node.Parent = srcNode->Parent;
			ConvertProperty3D((Property3DClassObject*)srcNode->Trans, node.Translation);
			ConvertProperty3D((Property3DClassObject*)srcNode->Rot, node.Rotation);
			ConvertProperty3D((Property3DClassObject*)srcNode->Scale, node.Scale);
			ConvertProperty1D((Property1DClassObject*)srcNode->Visibility, node.Visibility);
		}
	}

	for (Py_ssize_t i = 0; i < PyList_Size(self->ObjectHrcNameList); i++)
		a3d.ObjectHrcList.push_back(Narrow(PyList_GetItem(self->ObjectHrcNameList, i)));

	for (Py_ssize_t i = 0; i < PyList_Size(self->Objects); i++)
	{
		auto* src = (ObjectClassObject*)PyList_GetItem(self->Objects, i);
		auto& obj = a3d.Objects.emplace_back();

		obj.Name = Narrow(src->Name);
		obj.UIDName = Narrow(src->UIDName);
		ConvertProperty3D((Property3DClassObject*)src->Trans, obj.Translation);
		ConvertProperty3D((Property3DClassObject*)src->Rot, obj.Rotation);
		ConvertProperty3D((Property3DClassObject*)src->Scale, obj.Scale);
		ConvertProperty1D((Property1DClassObject*)src->Visibility, obj.Visibility);
	}

	for (Py_ssize_t i = 0; i < PyList_Size(self->ObjectNameList); i++)
		a3d.ObjectList.push_back(Narrow(PyList_GetItem(self->ObjectNameList, i)));

	IO::Writer w;
	switch (format)
	{
	case EXPORT_FORMAT_A3DA:
		a3d.Write(w);
		break;
	case EXPORT_FORMAT_A3DC:
		a3d.CompressF16 = static_cast<Auth::CompressF16>(compress);
		a3d.WriteCompressed(w);
		break;
	}

	if (!w.Flush(file))
		PyErr_SetString(PyExc_FileNotFoundError, "Can't write to this file!");

	return PyBool_FromLong(1);
};

static PyMethodDef Auth3d_methods[] = {
	{ "write_to_file",  (PyCFunction)AuthClassType_Write, METH_VARARGS, "Writes Auth3D object to the file." },
	{ nullptr }
};

static bool PreInitTypeObjects()
{
	AuthClassType.tp_name = "auth3d.Auth3d";
	AuthClassType.tp_basicsize = sizeof(AuthClassObject);
	AuthClassType.tp_itemsize = 0;
	AuthClassType.tp_flags = Py_TPFLAGS_DEFAULT;
	AuthClassType.tp_new = Auth3d_new;
	AuthClassType.tp_dealloc = (destructor)Auth3d_dealloc;
	AuthClassType.tp_members = Auth3d_members;
	AuthClassType.tp_methods = Auth3d_methods;

	ObjhrcClassType.tp_name = "auth3d.ObjectHrc";
	ObjhrcClassType.tp_basicsize = sizeof(ObjhrcClassObject);
	ObjhrcClassType.tp_itemsize = 0;
	ObjhrcClassType.tp_flags = Py_TPFLAGS_DEFAULT;
	ObjhrcClassType.tp_new = ObjectHrc_new;
	ObjhrcClassType.tp_members = ObjectHrc_members;

	Property1DClassType.tp_name = "auth3d.Property1d";
	Property1DClassType.tp_basicsize = sizeof(Property1DClassObject);
	Property1DClassType.tp_itemsize = 0;
	Property1DClassType.tp_flags = Py_TPFLAGS_DEFAULT;
	Property1DClassType.tp_new = PyType_GenericNew;
	Property1DClassType.tp_init = (initproc)Property1D_init;
	Property1DClassType.tp_members = Property1d_members;

	Property3DClassType.tp_name = "auth3d.Property3d";
	Property3DClassType.tp_basicsize = sizeof(Property3DClassObject);
	Property3DClassType.tp_itemsize = 0;
	Property3DClassType.tp_flags = Py_TPFLAGS_DEFAULT;
	Property3DClassType.tp_new = Property3d_new;
	Property3DClassType.tp_members = Property3d_members;

	HrcNodeClassType.tp_name = "auth3d.HrcNode";
	HrcNodeClassType.tp_basicsize = sizeof(HrcNodeClassObject);
	HrcNodeClassType.tp_itemsize = 0;
	HrcNodeClassType.tp_flags = Py_TPFLAGS_DEFAULT;
	HrcNodeClassType.tp_new = HrcNode_new;
	HrcNodeClassType.tp_members = HrcNode_members;

	ObjectClassType.tp_name = "auth3d.Object";
	ObjectClassType.tp_basicsize = sizeof(ObjectClassObject);
	ObjectClassType.tp_itemsize = 0;
	ObjectClassType.tp_flags = Py_TPFLAGS_DEFAULT;
	ObjectClassType.tp_new = Object_new;
	ObjectClassType.tp_members = Object_members;

	if (PyType_Ready(&AuthClassType) < 0) return false;
	if (PyType_Ready(&ObjhrcClassType) < 0) return false;
	if (PyType_Ready(&HrcNodeClassType) < 0) return false;
	if (PyType_Ready(&ObjectClassType) < 0) return false;
	if (PyType_Ready(&Property3DClassType) < 0) return false;
	if (PyType_Ready(&Property1DClassType) < 0) return false;

	return true;
}

static bool RegisterTypeObject(PyObject* mod, const char* name, PyTypeObject* type)
{
	Py_INCREF(type);
	if (PyModule_AddObject(mod, name, (PyObject*)type) < 0) {
		Py_DECREF(type);
		Py_DECREF(mod);
		return false;
	}

	return true;
}

static bool PostInitTypeObjects(PyObject* mod)
{
	if (!RegisterTypeObject(mod, "Auth3d", &AuthClassType)) return false;
	if (!RegisterTypeObject(mod, "ObjectHrc", &ObjhrcClassType)) return false;
	if (!RegisterTypeObject(mod, "HrcNode", &HrcNodeClassType)) return false;
	if (!RegisterTypeObject(mod, "Object", &ObjectClassType)) return false;
	if (!RegisterTypeObject(mod, "Property3d", &Property3DClassType)) return false;
	if (!RegisterTypeObject(mod, "Property1d", &Property1DClassType)) return false;

	return true;
}

static PyMethodDef ModuleMethods[] = {
	/*{ "create_auth", Factory::CreateAuth, METH_VARARGS, "Creates an Auth3D object and returns the handle." },
	{ "write_auth", WriteAuth, METH_VARARGS, "Write Auth3D object to file." },
	{ "set_auth_id", SetAuthIndex, METH_VARARGS, "Set the Auth3D id to modify."  },
	{ "reset_auth_id", ResetAuthIndex, METH_VARARGS, "Reset the Auth3D id to modify." },
	{ "create_auth_objhrc", Factory::CreateAuthObjectHrc, METH_VARARGS, "Add objhrc to Auth3D." },*/
	{ nullptr, nullptr, 0, nullptr }
};

static PyModuleDef ModuleInfo = {
	PyModuleDef_HEAD_INIT,
	"auth3d",
	nullptr,
	-1,
	ModuleMethods
};

PyMODINIT_FUNC PyInit_auth3d(void)
{
	PyObject* mod = nullptr;

	if (!PreInitTypeObjects())
		return nullptr;

	mod = PyModule_Create(&ModuleInfo);
	if (mod == nullptr)
		return nullptr;

	PostInitTypeObjects(mod);
	return mod;
}