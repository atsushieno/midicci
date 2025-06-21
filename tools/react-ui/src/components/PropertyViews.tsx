import { useState, useEffect } from 'react';
import { PropertyMetadata, PropertyColumn, PropertySetAccess } from '../types/midi';

interface PropertyColumnProps {
  label: string;
  children: React.ReactNode;
}

function PropertyColumnComponent({ label, children }: PropertyColumnProps) {
  return (
    <div className="flex items-center mb-4">
      <div className="w-48 p-3 bg-gray-50 rounded-l border">
        <span className="text-sm font-medium text-gray-700">{label}</span>
      </div>
      <div className="flex-1 p-3 border-t border-r border-b rounded-r">
        {children}
      </div>
    </div>
  );
}

interface PropertyListEntryProps {
  propertyId: string;
  isSelected: boolean;
  onSelect: (propertyId: string) => void;
}

function PropertyListEntry({ propertyId, isSelected, onSelect }: PropertyListEntryProps) {
  return (
    <button
      onClick={() => onSelect(propertyId)}
      className={`w-full p-3 text-left border rounded ${
        isSelected 
          ? 'border-blue-500 bg-blue-50 text-blue-900' 
          : 'border-gray-300 hover:border-gray-400 hover:bg-gray-50'
      }`}
    >
      {propertyId}
    </button>
  );
}

interface PropertyMetadataEditorProps {
  metadata: PropertyMetadata;
  onUpdate: (metadata: PropertyMetadata) => void;
  readOnly?: boolean;
}

function PropertyMetadataEditor({ metadata, onUpdate, readOnly = false }: PropertyMetadataEditorProps) {
  const [formData, setFormData] = useState<PropertyMetadata>(metadata);
  const [columns, setColumns] = useState<PropertyColumn[]>(metadata.columns || []);

  useEffect(() => {
    setFormData(metadata);
    setColumns(metadata.columns || []);
  }, [metadata]);

  const handleUpdate = () => {
    const updatedMetadata = {
      ...formData,
      mediaTypes: formData.mediaTypes,
      encodings: formData.encodings,
      columns
    };
    onUpdate(updatedMetadata);
  };

  const handleMediaTypesChange = (value: string) => {
    setFormData(prev => ({
      ...prev,
      mediaTypes: value.split('\n').filter(t => t.trim())
    }));
  };

  const handleEncodingsChange = (value: string) => {
    setFormData(prev => ({
      ...prev,
      encodings: value.split('\n').filter(e => e.trim())
    }));
  };

  const addColumn = () => {
    setColumns(prev => [...prev, { title: '', link: '', property: '' }]);
  };

  const removeColumn = (index: number) => {
    setColumns(prev => prev.filter((_, i) => i !== index));
  };

  const updateColumn = (index: number, field: keyof PropertyColumn, value: string) => {
    setColumns(prev => prev.map((col, i) => 
      i === index ? { ...col, [field]: value } : col
    ));
  };

  return (
    <div className="space-y-4">
      <div className="flex items-center justify-between">
        <h3 className="text-xl font-bold text-gray-900">Property Metadata</h3>
        {!readOnly && (
          <button
            onClick={handleUpdate}
            className="bg-blue-600 hover:bg-blue-700 text-white px-4 py-2 rounded-md"
          >
            Update Metadata
          </button>
        )}
      </div>

      <PropertyColumnComponent label="resource">
        <input
          type="text"
          value={formData.resource}
          onChange={(e) => setFormData(prev => ({ ...prev, resource: e.target.value }))}
          readOnly={readOnly}
          className="w-full px-3 py-2 border border-gray-300 rounded-md focus:outline-none focus:ring-blue-500 focus:border-blue-500"
        />
      </PropertyColumnComponent>

      <PropertyColumnComponent label="canGet">
        <input
          type="checkbox"
          checked={formData.canGet}
          onChange={(e) => setFormData(prev => ({ ...prev, canGet: e.target.checked }))}
          disabled={readOnly}
          className="h-4 w-4 text-blue-600 focus:ring-blue-500 border-gray-300 rounded"
        />
      </PropertyColumnComponent>

      <PropertyColumnComponent label="canSet">
        <PropertySetAccessSelector
          value={formData.canSet}
          onChange={(value) => setFormData(prev => ({ ...prev, canSet: value }))}
          readOnly={readOnly}
        />
      </PropertyColumnComponent>

      <PropertyColumnComponent label="canSubscribe">
        <input
          type="checkbox"
          checked={formData.canSubscribe}
          onChange={(e) => setFormData(prev => ({ ...prev, canSubscribe: e.target.checked }))}
          disabled={readOnly}
          className="h-4 w-4 text-blue-600 focus:ring-blue-500 border-gray-300 rounded"
        />
      </PropertyColumnComponent>

      <PropertyColumnComponent label="requireResId">
        <input
          type="checkbox"
          checked={formData.requireResId}
          onChange={(e) => setFormData(prev => ({ ...prev, requireResId: e.target.checked }))}
          disabled={readOnly}
          className="h-4 w-4 text-blue-600 focus:ring-blue-500 border-gray-300 rounded"
        />
      </PropertyColumnComponent>

      <PropertyColumnComponent label="mediaTypes">
        <textarea
          value={formData.mediaTypes.join('\n')}
          onChange={(e) => handleMediaTypesChange(e.target.value)}
          readOnly={readOnly}
          rows={3}
          className="w-full px-3 py-2 border border-gray-300 rounded-md focus:outline-none focus:ring-blue-500 focus:border-blue-500"
        />
      </PropertyColumnComponent>

      <PropertyColumnComponent label="encodings">
        <textarea
          value={formData.encodings.join('\n')}
          onChange={(e) => handleEncodingsChange(e.target.value)}
          readOnly={readOnly}
          rows={3}
          className="w-full px-3 py-2 border border-gray-300 rounded-md focus:outline-none focus:ring-blue-500 focus:border-blue-500"
        />
      </PropertyColumnComponent>

      <PropertyColumnComponent label="schema">
        <textarea
          value={formData.schema || ''}
          onChange={(e) => setFormData(prev => ({ ...prev, schema: e.target.value || undefined }))}
          readOnly={readOnly}
          rows={5}
          className="w-full px-3 py-2 border border-gray-300 rounded-md focus:outline-none focus:ring-blue-500 focus:border-blue-500"
        />
      </PropertyColumnComponent>

      <PropertyColumnComponent label="canPaginate">
        <input
          type="checkbox"
          checked={formData.canPaginate}
          onChange={(e) => setFormData(prev => ({ ...prev, canPaginate: e.target.checked }))}
          disabled={readOnly}
          className="h-4 w-4 text-blue-600 focus:ring-blue-500 border-gray-300 rounded"
        />
      </PropertyColumnComponent>

      <PropertyColumnComponent label="columns">
        <div className="space-y-3">
          {columns.map((column, index) => (
            <div key={index} className="border rounded-lg p-3 space-y-2">
              <div className="flex items-center space-x-2">
                <input
                  type="text"
                  value={column.title}
                  onChange={(e) => updateColumn(index, 'title', e.target.value)}
                  placeholder="Column title"
                  readOnly={readOnly}
                  className="flex-1 px-3 py-2 border border-gray-300 rounded-md focus:outline-none focus:ring-blue-500 focus:border-blue-500"
                />
                {!readOnly && (
                  <button
                    onClick={() => removeColumn(index)}
                    className="text-red-600 hover:text-red-800 p-2"
                  >
                    ✕
                  </button>
                )}
              </div>
              <div className="flex items-center space-x-2">
                <label className="flex items-center">
                  <input
                    type="checkbox"
                    checked={!!column.link}
                    onChange={(e) => {
                      if (e.target.checked) {
                        updateColumn(index, 'link', column.property || '');
                        updateColumn(index, 'property', '');
                      } else {
                        updateColumn(index, 'property', column.link || '');
                        updateColumn(index, 'link', '');
                      }
                    }}
                    disabled={readOnly}
                    className="mr-2"
                  />
                  Link?
                </label>
                <input
                  type="text"
                  value={column.link || column.property || ''}
                  onChange={(e) => {
                    if (column.link !== undefined) {
                      updateColumn(index, 'link', e.target.value);
                    } else {
                      updateColumn(index, 'property', e.target.value);
                    }
                  }}
                  placeholder={column.link !== undefined ? "Link URL" : "Property ID"}
                  readOnly={readOnly}
                  className="flex-1 px-3 py-2 border border-gray-300 rounded-md focus:outline-none focus:ring-blue-500 focus:border-blue-500"
                />
              </div>
            </div>
          ))}
          {!readOnly && (
            <button
              onClick={addColumn}
              className="w-full p-3 border-2 border-dashed border-gray-300 rounded-lg text-gray-600 hover:border-gray-400 hover:text-gray-800"
            >
              + Add Column
            </button>
          )}
        </div>
      </PropertyColumnComponent>
    </div>
  );
}

interface PropertySetAccessSelectorProps {
  value: PropertySetAccess;
  onChange: (value: PropertySetAccess) => void;
  readOnly?: boolean;
}

function PropertySetAccessSelector({ value, onChange, readOnly = false }: PropertySetAccessSelectorProps) {
  return (
    <select
      value={value}
      onChange={(e) => onChange(e.target.value as PropertySetAccess)}
      disabled={readOnly}
      className="w-full px-3 py-2 border border-gray-300 rounded-md focus:outline-none focus:ring-blue-500 focus:border-blue-500"
    >
      <option value={PropertySetAccess.NONE}>none</option>
      <option value={PropertySetAccess.FULL}>full</option>
      <option value={PropertySetAccess.PARTIAL}>partial</option>
    </select>
  );
}

interface PropertyEncodingSelectorProps {
  encodings: string[];
  selectedEncoding: string;
  onSelectionChange: (encoding: string) => void;
}

function PropertyEncodingSelector({ encodings, selectedEncoding, onSelectionChange }: PropertyEncodingSelectorProps) {
  return (
    <select
      value={selectedEncoding}
      onChange={(e) => onSelectionChange(e.target.value)}
      className="px-3 py-2 border border-gray-300 rounded-md focus:outline-none focus:ring-blue-500 focus:border-blue-500"
    >
      <option value="">-- Select Encoding --</option>
      {encodings.map((encoding) => (
        <option key={encoding} value={encoding}>
          {encoding}
        </option>
      ))}
    </select>
  );
}

interface PropertyValueEditorProps {
  isLocalEditor: boolean;
  mediaType: string;
  metadata?: PropertyMetadata;
  body: Uint8Array;
  isSubscribing: boolean;
  onRefreshValue: (encoding?: string, offset?: number, limit?: number) => void;
  onSubscriptionChange: (subscribing: boolean, encoding?: string) => void;
  onCommitChange: (data: Uint8Array, resId?: string, encoding?: string, isPartial?: boolean) => void;
}

function PropertyValueEditor({
  isLocalEditor,
  mediaType,
  metadata,
  body,
  isSubscribing,
  onRefreshValue,
  onSubscriptionChange,
  onCommitChange
}: PropertyValueEditorProps) {
  const [editing, setEditing] = useState(false);
  const [resId, setResId] = useState('');
  const [selectedEncoding, setSelectedEncoding] = useState<string>('');
  const [text, setText] = useState('');
  const [paginateOffset, setPaginateOffset] = useState('0');
  const [paginateLimit, setPaginateLimit] = useState('9999');

  useEffect(() => {
    if (mediaType === 'application/json') {
      setText(new TextDecoder().decode(body));
    }
    setEditing(false);
    setSelectedEncoding('');
  }, [body, mediaType]);

  const isEditableByMetadata = metadata?.canSet !== PropertySetAccess.NONE;
  const isEditable = isLocalEditor || isEditableByMetadata;
  const isTextRenderable = mediaType === 'application/json';

  const handleCommitChanges = () => {
    if (isTextRenderable) {
      const encoder = new TextEncoder();
      const data = encoder.encode(text);
      onCommitChange(data, resId || undefined, selectedEncoding || undefined, false);
    }
    setEditing(false);
  };

  const handleRefresh = () => {
    const offset = metadata?.canPaginate ? parseInt(paginateOffset) || undefined : undefined;
    const limit = metadata?.canPaginate ? parseInt(paginateLimit) || undefined : undefined;
    onRefreshValue(selectedEncoding || undefined, offset, limit);
  };

  return (
    <div className="space-y-4">
      <h3 className="text-xl font-bold text-gray-900">Property Value</h3>

      {!isLocalEditor && editing && (
        <div className="flex items-center space-x-2">
          <label className="text-sm font-medium text-gray-700">resId (if applicable):</label>
          <input
            type="text"
            value={resId}
            onChange={(e) => setResId(e.target.value)}
            className="px-3 py-2 border border-gray-300 rounded-md focus:outline-none focus:ring-blue-500 focus:border-blue-500"
          />
        </div>
      )}

      {isTextRenderable ? (
        <div className="space-y-4">
          {isEditable && (
            <div className="flex items-center space-x-2">
              <input
                type="checkbox"
                checked={editing}
                onChange={(e) => setEditing(e.target.checked)}
                className="h-4 w-4 text-blue-600 focus:ring-blue-500 border-gray-300 rounded"
              />
              <label className="text-sm font-medium text-gray-700">Edit</label>
            </div>
          )}

          {editing ? (
            <div className="space-y-4">
              <textarea
                value={text}
                onChange={(e) => setText(e.target.value)}
                rows={10}
                className="w-full px-3 py-2 border border-gray-300 rounded-md focus:outline-none focus:ring-blue-500 focus:border-blue-500 font-mono text-sm"
              />
              <div className="flex space-x-2">
                <button
                  onClick={handleCommitChanges}
                  className="bg-green-600 hover:bg-green-700 text-white px-4 py-2 rounded-md"
                >
                  Commit Changes
                </button>
                <PropertyEncodingSelector
                  encodings={metadata?.encodings || []}
                  selectedEncoding={selectedEncoding}
                  onSelectionChange={setSelectedEncoding}
                />
              </div>
            </div>
          ) : (
            <div className="space-y-4">
              <PropertyRefreshAndSubscribeButtons
                isLocalEditor={isLocalEditor}
                editing={editing}
                isSubscribing={isSubscribing}
                metadata={metadata}
                paginateOffset={paginateOffset}
                paginateLimit={paginateLimit}
                selectedEncoding={selectedEncoding}
                onRefresh={handleRefresh}
                onSubscriptionChange={onSubscriptionChange}
                onPaginateOffsetChange={setPaginateOffset}
                onPaginateLimitChange={setPaginateLimit}
                onEncodingChange={setSelectedEncoding}
              />
              <textarea
                value={text}
                readOnly
                rows={10}
                className="w-full px-3 py-2 border border-gray-300 rounded-md bg-gray-50 font-mono text-sm"
              />
            </div>
          )}
        </div>
      ) : (
        <div className="space-y-4">
          <p className="text-gray-600">MIME type '{mediaType}' not supported for editing</p>
          <PropertyRefreshAndSubscribeButtons
            isLocalEditor={isLocalEditor}
            editing={editing}
            isSubscribing={isSubscribing}
            metadata={metadata}
            paginateOffset={paginateOffset}
            paginateLimit={paginateLimit}
            selectedEncoding={selectedEncoding}
            onRefresh={handleRefresh}
            onSubscriptionChange={onSubscriptionChange}
            onPaginateOffsetChange={setPaginateOffset}
            onPaginateLimitChange={setPaginateLimit}
            onEncodingChange={setSelectedEncoding}
          />
        </div>
      )}
    </div>
  );
}

interface PropertyRefreshAndSubscribeButtonsProps {
  isLocalEditor: boolean;
  editing: boolean;
  isSubscribing: boolean;
  metadata?: PropertyMetadata;
  paginateOffset: string;
  paginateLimit: string;
  selectedEncoding: string;
  onRefresh: () => void;
  onSubscriptionChange: (subscribing: boolean, encoding?: string) => void;
  onPaginateOffsetChange: (offset: string) => void;
  onPaginateLimitChange: (limit: string) => void;
  onEncodingChange: (encoding: string) => void;
}

function PropertyRefreshAndSubscribeButtons({
  isLocalEditor,
  editing,
  isSubscribing,
  metadata,
  paginateOffset,
  paginateLimit,
  selectedEncoding,
  onRefresh,
  onSubscriptionChange,
  onPaginateOffsetChange,
  onPaginateLimitChange,
  onEncodingChange
}: PropertyRefreshAndSubscribeButtonsProps) {
  if (isLocalEditor || editing) {
    return null;
  }

  return (
    <div className="space-y-4">
      <div className="flex items-center space-x-2">
        <button
          onClick={onRefresh}
          className="bg-blue-600 hover:bg-blue-700 text-white px-4 py-2 rounded-md"
        >
          Refresh
        </button>
        
        {metadata?.canSubscribe && (
          <button
            onClick={() => onSubscriptionChange(!isSubscribing, selectedEncoding || undefined)}
            className={`px-4 py-2 rounded-md text-white ${
              isSubscribing 
                ? 'bg-red-600 hover:bg-red-700' 
                : 'bg-green-600 hover:bg-green-700'
            }`}
          >
            {isSubscribing ? 'Unsubscribe' : 'Subscribe'}
          </button>
        )}

        <PropertyEncodingSelector
          encodings={metadata?.encodings || []}
          selectedEncoding={selectedEncoding}
          onSelectionChange={onEncodingChange}
        />
      </div>

      {metadata?.canPaginate && (
        <div className="flex items-center space-x-2">
          <span className="text-sm text-gray-700">Paginate? offset:</span>
          <input
            type="number"
            value={paginateOffset}
            onChange={(e) => onPaginateOffsetChange(e.target.value)}
            className="w-20 px-2 py-1 border border-gray-300 rounded text-sm"
          />
          <span className="text-sm text-gray-700">limit:</span>
          <input
            type="number"
            value={paginateLimit}
            onChange={(e) => onPaginateLimitChange(e.target.value)}
            className="w-20 px-2 py-1 border border-gray-300 rounded text-sm"
          />
        </div>
      )}
    </div>
  );
}

export {
  PropertyColumnComponent,
  PropertyListEntry,
  PropertyMetadataEditor,
  PropertySetAccessSelector,
  PropertyEncodingSelector,
  PropertyValueEditor,
  PropertyRefreshAndSubscribeButtons
};
